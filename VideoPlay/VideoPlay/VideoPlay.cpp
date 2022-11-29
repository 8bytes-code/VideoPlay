#include <iostream>
#include "vlc.h"
#include <Windows.h>
#include <conio.h>


std::string UnicodeToUtf8(const std::wstring& strIn) {
	std::string strOut;
	//利用这个函数先获取到转换之后的长度
	int length = ::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), NULL, 0, NULL, NULL);
	strOut.resize(length + 1);
	//再把转换的内容传递给strout
	::WideCharToMultiByte(CP_UTF8, 0, strIn.c_str(), strIn.size(), (LPSTR)strOut.c_str(), length, NULL, NULL);

	return strOut;
}

int main(){
	int argc = 1;
	char* argv[2];
	argv[0] = (char*)"--ignore-config";		//用于忽略vlc软件的全局配置

	libvlc_instance_t* vlc_ins = libvlc_new(argc, argv);
	//vlc的路径都是utf8，vs默认是多字节，需要先从多字节转换成unicode，再转换成utf8
	std::string path = UnicodeToUtf8(L"股市讨论.mp4");
	libvlc_media_t* media = libvlc_media_new_path(vlc_ins, path.c_str());
	//media = libvlc_media_new_location(vlc_ins, "file:///E:\\VideoPlay\\股市讨论.mp4");		两种加载方法
	libvlc_media_player_t* player = libvlc_media_player_new_from_media(media);

	//播放
	do {
		int ret = libvlc_media_player_play(player);
		if (ret == -1) {
			printf("player error!\r\n");
			break;
		} 
		Sleep(300);	//貌似解析太快了，后面的东西都没拿到数据
		
		//音量 想办法给点时间解析
		int vol = -1;
		while (vol == -1) {
			Sleep(10);
			vol = libvlc_audio_get_volume(player);
		}
		printf("volume is %d\r\n", vol);
		libvlc_audio_set_volume(player, 50);	//默认是100，0变相的就是静音

		//获取长度
		libvlc_time_t tm = libvlc_media_player_get_length(player);
		printf("%02d:%02d:%02d.%03d\r\n", int(tm / 3600000), int(tm / 60000) % 60, int(tm / 1000) % 60, int(tm) % 1000);
		//获取宽高
		int width = libvlc_video_get_width(player);
		int height = libvlc_video_get_height(player);
		printf("width:%d height:%d\r\n", width, height);  
		
		while (!_kbhit()) {
			//播放进度
			printf("%f%%\r", 100.0 * libvlc_media_player_get_position(player));
			Sleep(500);
		}

		//按一个键暂停 在按一个键播放 再按一个停止播放
		getchar();
		libvlc_media_player_pause(player);
		getchar();
		libvlc_media_player_play(player);
		getchar();
		libvlc_media_player_stop(player);
	} while (0);
	
	//释放资源
	libvlc_media_player_release(player);	//播放器
	libvlc_media_release(media);			//媒体
	libvlc_release(vlc_ins);				//实例

	return 0;
}
