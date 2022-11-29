#pragma once
#include <Windows.h>
#include <atomic>
#include <vector>
#include <mutex>
#include <varargs.h>

class HTool {
public:
	static void HTrace(const char* fotmat, ...) {
		va_list ap;
		va_start(ap, fotmat);
		std::string sBuffer;
		sBuffer.resize(1034 * 10);
		vsprintf((char*)(sBuffer.c_str()), fotmat, ap);
		OutputDebugStringA(sBuffer.c_str());
		va_end(ap);
	}
};
/*没有trace就用自己模拟的，利用可变参数*/

#ifndef TRACE
#define TRACE HTool::HTrace
#endif

class ThreadFuncBase {};
typedef int (ThreadFuncBase::* FUNCTYPE)();
class ThreadWorker{
public:
	ThreadWorker():thiz(NULL), func(NULL) {}
	ThreadWorker(void* obj, FUNCTYPE f): thiz((ThreadFuncBase*)obj), func(f) {}
	ThreadWorker(const ThreadWorker& worker) {
		thiz = worker.thiz;
		func = worker.func;
	}
	ThreadWorker& operator=(const ThreadWorker& worker) {
		if (this != &worker) {
			thiz = worker.thiz;
			func = worker.func;
		}
		return *this;
	}
	int operator()() {
		if (IsValid()) {
			return (thiz->*func)();
		}
		return -1;
	}
	bool IsValid() const{
		return (thiz != NULL) && (func != NULL);//true 没问题
	}
private:
	ThreadFuncBase* thiz;
	FUNCTYPE func;
};

class HeThread {
public:
	HeThread() {
		m_hThread = NULL;
		m_bStatus = false;
	}

	~HeThread() {
		Stop();
	}

	bool Start() {
		m_bStatus = true;
		m_hThread = (HANDLE)_beginthread(&HeThread::ThreadEntry, 0, this);
		if (!IsValid()) m_bStatus = false;
		return m_bStatus;
	}

	//true成功 false失败
	bool Stop() {
		if (m_bStatus == false) return true;
		m_bStatus = false;
		DWORD ret = WaitForSingleObject(m_hThread, 1000);
		if (ret == WAIT_TIMEOUT) {
			//强制终止
			TerminateThread(m_hThread, -1);
		}
		UpdateWorker();
		return ret == WAIT_OBJECT_0;
	}

	bool IsValid() {
		//如果线程结束了或者不存在都将返回其他值 true才表示线程有效
		if (m_hThread == NULL || (m_hThread == INVALID_HANDLE_VALUE))return false;
		return WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT;
	}

	void UpdateWorker(const ::ThreadWorker& worker = ::ThreadWorker()) {
		//哎，内存泄露，需要注意，任务不为空且不重复才可以进行
		if (m_worker.load() != NULL && (m_worker.load() != &worker)) {
			::ThreadWorker* pWorker = m_worker.load();
			m_worker.store(NULL);
			delete pWorker;
		}
		if (m_worker.load() == &worker) return;
		if (!worker.IsValid()) {
			m_worker.store(NULL);
			return;
		}
		::ThreadWorker* pWorker = new ::ThreadWorker(worker);
		TRACE("new pWorker=%08X m_worker=%08X\r\n", pWorker, m_worker.load());
		m_worker.store(pWorker);
	}

	bool Isdle() {
		//返回ture表示空闲，false表示已经有任务了
		if (m_worker.load() == NULL)return true;
		return !m_worker.load()->IsValid();
	}
private:
	void ThreadWorker() {
		while (m_bStatus) {
			if (m_worker.load() == NULL) {
				Sleep(1);
				continue;
			}
			::ThreadWorker worker = *m_worker.load();
			if (worker.IsValid()) {
				if (WaitForSingleObject(m_hThread, 0) == WAIT_TIMEOUT) {
					int ret = worker();
					if (ret != 0) {
						TRACE("thread found warning code %d\r\n", ret);
					}
					if (ret < 0) {
						//存入一个无效的值
						::ThreadWorker* pWorker = m_worker.load();
						m_worker.store(NULL);
						delete pWorker;
					}
				}
			}
			else {
				Sleep(1);
			}
		}
	}

	static void ThreadEntry(void* arg) {
		HeThread* thiz = (HeThread*)arg;
		if (thiz) {
			thiz->ThreadWorker();
		}
		_endthread();
	}
private:
	HANDLE m_hThread;
	bool m_bStatus;	//false表示线程将要关闭 true表示线程正在进行
	std::atomic<::ThreadWorker*> m_worker;
};

class HeThreadPool {
public:
	HeThreadPool(size_t size) {
		m_thread.resize(size);
		for (size_t i = 0; i < size; i++) {
			m_thread[i] = new HeThread();
		}
	}
	HeThreadPool(){}
	~HeThreadPool(){
		Stop();
		//由于构造时是通过new的，且没有啥基类，要靠自己释放
		for (size_t i = 0; i < m_thread.size(); i++) {
			HeThread* pThread = m_thread[i];
			m_thread[i] = NULL;
			delete pThread;
		}
		m_thread.clear();
	}

	//线程池只有全都启动才算成功
	bool Invoke() {
		bool ret = true;
		for (size_t i = 0; i < m_thread.size(); i++) {
			if (m_thread[i]->Start() == false) {
				ret = false;
				break;
			}
		}
		if (ret == false) {
			for (size_t i = 0; i < m_thread.size(); i++) {
				m_thread[i]->Stop();
			}
		}
		return ret;
	}

	void Stop() {
		for (size_t i = 0; i < m_thread.size(); i++) {
			m_thread[i]->Stop();
		}
	}

	/*TODO: 若线程量巨大，加上互斥锁，分发任务效果会变差，更何况有任务的线程比较零散，就还得套循环，此处有优化空间*/
	//分发任务
	int DispatchWorker(const ThreadWorker& worker) {
		int index = -1;		//标记工作线程
		m_lock.lock();
		for (size_t i = 0; i < m_thread.size(); i++) {
			if (m_thread[i]->Isdle()) {
				m_thread[i]->UpdateWorker(worker);
				index = i;
				break;
			}
		}
		m_lock.unlock();
		//如果返回-1则代表所有线程都有任务，返回非-1的值说明那个线程有任务
		return index;
	}

	//检测线程有效性
	bool CheckThreadValid(size_t index) {
		if (index < m_thread.size()) {
			return m_thread[index]->IsValid();
		}
		return false;
	}
private:
	std::mutex m_lock;
	std::vector<HeThread*> m_thread;
};