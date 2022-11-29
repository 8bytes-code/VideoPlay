#pragma once
#include "HeThread.h"
#include <list>
#include <atomic>

template<class T>
class CHeQueue {
public:
	enum {
		HQNone,
		HQPush,
		HQPop,
		HQSize,
		HQClear
	};

	typedef struct IocpParam {
		size_t nOperator;			//操作 附带记录size
		T Data;						//数据
		HANDLE hEvent;				//pop时机
		
		
		IocpParam(int op, typename const T& data, HANDLE hEve = NULL) {
			nOperator = op;
			Data = data;
			hEvent = hEve;
		}
		IocpParam() {
			nOperator = HQNone;
		}
	}PPARAM;	//Post Parameter 用于投递信息的结构体
	
//线程安全的队列(利用IOCP实现)
public:
	CHeQueue() {
		m_lock = false;
		m_hCompeletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);
		m_hThread = INVALID_HANDLE_VALUE;
		if (m_hCompeletionPort != NULL) {
			m_hThread = (HANDLE)_beginthread(&CHeQueue<T>::theradEntry, 0, this);
		}
	}

	virtual ~CHeQueue() {
		if (m_lock)return;
		m_lock = true;
		PostQueuedCompletionStatus(m_hCompeletionPort, 0, NULL, NULL);
		WaitForSingleObject(m_hThread, INFINITE);
		if (m_hCompeletionPort != NULL) {
			HANDLE hTemp = m_hCompeletionPort;
			m_hCompeletionPort = NULL;
			CloseHandle(hTemp);
		}
	}

	bool PushBack(const T& data) {
		IocpParam* pParam = new IocpParam(HQPush, data);
		if (m_lock) {
			delete pParam;
			return false;
		}

		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		return ret;
	}

	virtual bool PopFront(T& data) {
		//pop的时候期望要标记这个值，老办法用事件
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(HQPop, data, hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return false;
		}

		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			CloseHandle(hEvent);
			return false;
		}

		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			data = Param.Data;
		}

		return ret;
	}

	size_t Size() {
		//确保线程之中能取到正确的size，也要考虑安全
		HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		IocpParam Param(HQSize, T(), hEvent);
		if (m_lock) {
			if (hEvent) CloseHandle(hEvent);
			return -1;
		}

		BOOL ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == FALSE) {
			CloseHandle(hEvent);
			return -1;
		}

		ret = WaitForSingleObject(hEvent, INFINITE) == WAIT_OBJECT_0;
		if (ret) {
			return Param.nOperator;
		}

		return -1;
	}

	bool Clear() {
		if (m_lock) return false;
		IocpParam* pParam = new IocpParam(HQClear, T());
		bool ret = PostQueuedCompletionStatus(m_hCompeletionPort, sizeof(PPARAM), (ULONG_PTR)pParam, NULL);
		if (ret == false)delete pParam;
		return ret;
	}

protected:
	static void theradEntry(void* arg) {
		CHeQueue<T>* thiz = (CHeQueue<T>*)arg;
		thiz->threadMain();
		_endthread();
	}

	virtual void DealParam(PPARAM* pParam) {
		switch (pParam->nOperator) {
			case HQPush:
				m_lstData.push_back(pParam->Data);
				delete pParam;
				break;
			case HQPop:
				if (m_lstData.size() > 0) {
					pParam->Data = m_lstData.front();
					m_lstData.pop_front();
				}
				if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
				break;
			case HQSize:
				pParam->nOperator = m_lstData.size();
				if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
				break;
			case HQClear:
				m_lstData.clear();
				delete pParam;
				break;
			default:
				TRACE("[%s]:threadMain error!\r\n", __FUNCTION__);
				break;
		}
	}

	void threadMain() {
		PPARAM* pParam = NULL;
		DWORD dwTransferred = 0;
		ULONG_PTR CompletionKey = 0;
		OVERLAPPED* pOverlapped = NULL;
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, INFINITE)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				break;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		//预防超时
		while (GetQueuedCompletionStatus(m_hCompeletionPort, &dwTransferred, &CompletionKey, &pOverlapped, 0)) {
			if ((dwTransferred == 0) || (CompletionKey == NULL)) {
				printf("thread is prepare to exit!\r\n");
				continue;
			}
			pParam = (PPARAM*)CompletionKey;
			DealParam(pParam);
		}
		HANDLE hTemp = m_hCompeletionPort;
		m_hCompeletionPort = NULL;
		CloseHandle(hTemp);
	}

protected:
	std::list<T> m_lstData;
	HANDLE m_hCompeletionPort;
	HANDLE m_hThread;
	std::atomic<bool> m_lock;				//队列析构
};


template<class T>
class HeSendQueue :public CHeQueue<T>,public ThreadFuncBase {
public:
	typedef int(ThreadFuncBase::* HECALLBACK)(T& data);

	HeSendQueue(ThreadFuncBase* obj, HECALLBACK callback)
		:CHeQueue<T>(), m_base(obj), m_callback(callback)
	{
		m_thread.Start();
		m_thread.UpdateWorker(::ThreadWorker(this, (FUNCTYPE)&HeSendQueue<T>::threadTick));
	}

	virtual	~HeSendQueue() {
		m_base = NULL;
		m_callback = NULL;
		m_thread.Stop();
	}
protected:
	virtual bool PopFront(T& data) {
		return false;
	}
	bool PopFront() {
		typename CHeQueue<T>::IocpParam* Param = new  typename CHeQueue<T>::IocpParam(CHeQueue<T>::HQPop, T());
		if (CHeQueue<T>::m_lock) {
			delete Param;
			return false;
		}
		bool ret = PostQueuedCompletionStatus(CHeQueue<T>::m_hCompeletionPort, sizeof(typename CHeQueue<T>::PPARAM), (ULONG_PTR)&Param, NULL);
		if (ret == false) {
			delete Param;
			return false;
		}
		return ret;
	}

	int threadTick() {
		if (WaitForSingleObject(CHeQueue<T>::m_hThread, 0) != WAIT_TIMEOUT) {
			return 0;
		}
		if (CHeQueue<T>::m_lstData.size() > 0) {
			PopFront();
		}
		//Sleep(1);
		return 0;
	}

	virtual void DealParam(typename CHeQueue<T>::PPARAM* pParam) {
		switch (pParam->nOperator) {
			case CHeQueue<T>::HQPush:
				CHeQueue<T>::m_lstData.push_back(pParam->Data);
				delete pParam;
				break;
			case CHeQueue<T>::HQPop:
				if (CHeQueue<T>::m_lstData.size() > 0) {
					pParam->Data = CHeQueue<T>::m_lstData.front();
					if((m_base->*m_callback)(pParam->Data)==0)
						CHeQueue<T>::m_lstData.pop_front();
				}
				delete pParam;
				break;
			case CHeQueue<T>::HQSize:
				pParam->nOperator = CHeQueue<T>::m_lstData.size();
				if (pParam->hEvent != NULL) SetEvent(pParam->hEvent);
				break;
			case CHeQueue<T>::HQClear:
				CHeQueue<T>::m_lstData.clear();
				delete pParam;
				break;
			default:
				OutputDebugStringA("unknown operator!\r\n");
				break;
		}
	}
private:
	ThreadFuncBase* m_base;
	HECALLBACK m_callback;
	HeThread m_thread;
};

typedef HeSendQueue<std::vector<char>>::HECALLBACK SENDCALLBACK;