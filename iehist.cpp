/*
   IE History Viewer
   Copyright (c) 2003 Patrik Karlsson

   http://www.cqure.net

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <winbase.h>
#include <winnt.h>
#include <time.h>


const int TYPE_URL  = 0x01;
const int TYPE_REDR = 0x02;

const int URL_URL_OFFSET  = 104;  // 0x68 Next, we see that the actual URL the user visited is located at offset 0x68 from the beginning o
const int URL_TIME_OFFSET = 16;  // Last accessed Time Stamp
const int REDR_URL_OFFSET = 16;  

const int Download_URL_OFFSET = 468; // 0x1D4;

// History, COokie, Caches 파싱 구조체
struct history {
	int nType;
	char *pURL;
	FILETIME st;
};

struct history_saved {
	int nType;
	char pURL[1024];
	//FILETIME st;
	unsigned __int64 epoch;
};

// IEHistoryDownload - index.dat Parsing Struct 
struct history_download {
	int nType;
	char *pURL;
	FILETIME st;
	WCHAR *pReferer;
	WCHAR *pDownloadURL;
	WCHAR *pLocation;

	//char *Referer;
	//char *DownloadURL;
	//char *pLocation;
};

struct history_download_saved {
	int nType;
	char pURL[1024];
	//FILETIME st;
	unsigned __int64 epoch;
	CHAR pReferer[1024];
	CHAR pDownloadURL[1024];
	CHAR pLocation[1024];
};


void FileTimeToUnixTime(LPFILETIME pft, unsigned __int64 * pt) {
    LONGLONG ll; // 64 bit value
    ll = (((LONGLONG)(pft->dwHighDateTime)) << 32) + pft->dwLowDateTime;
    *pt = (time_t)((ll - 116444736000000000L) / 10000000L);
}

int bMatchPattern( char *pBuf ) {

	// 
	//if ( pBuf[0] == 0x55 && pBuf[1] == 0x52 && pBuf[2] == 0x4c && pBuf[3] == 0x20 && pBuf[5] == 0x00 && pBuf[6] == 0x00)
	if ( pBuf[0] == 0x55 && pBuf[1] == 0x52 && pBuf[2] == 0x4c && pBuf[3] == 0x20 )
	{
		return TYPE_URL;
	}

	
	if ( pBuf[0] == 0x52 && pBuf[1] == 0x45 && pBuf[2] == 0x44 && pBuf[3] == 0x52 ) 
	{
		return TYPE_REDR;
	}

	return 0;
	
}

struct history_download *getDownload( char *pBuf, int nType ) {

	int i = 0;
	int j = 0;
	int k = 0;
	int l = 0;
	int a;

	struct history_download *pDownlaod;
	FILETIME ft;

	if ( nType == TYPE_URL ) {

		pDownlaod = (struct history_download*) malloc ( sizeof( struct history_download ) );

		// Last accessed Time Stamp 구조체에 저장.
		memcpy( (DWORD *)&ft.dwLowDateTime, pBuf + URL_TIME_OFFSET, sizeof( DWORD ) );
		memcpy( (DWORD *)&ft.dwHighDateTime, pBuf + URL_TIME_OFFSET + 4, sizeof( DWORD ) );
		                                 
		//FileTimeToSystemTime( &ft, &pDownlaod->st );
		pDownlaod->st = ft;
		pDownlaod->nType = TYPE_URL;

		pBuf += URL_URL_OFFSET;

	}
	// REDR 속성 파일 일단 no-Touch
	else if ( nType == TYPE_REDR ) {
		pDownlaod = (struct history_download*) malloc ( sizeof( struct history_download ) );
		ft.dwHighDateTime = 0;
		ft.dwLowDateTime = 0;
		//FileTimeToSystemTime( &ft, &pDownlaod->st );
		pDownlaod->st = ft;
		pDownlaod->nType = TYPE_REDR;
		pBuf += REDR_URL_OFFSET;
	}

	else {
		return NULL;
	}

	// ------------ GUID Value Parsing Start
	// 현재 위치 (실제 URL이 저장된 위치에서 부터 NULL이 나오는 부분까지의 길이를 구하기 위해서 i를 증가함)
	while ( pBuf[i] != 0 ) {  
		i++;
	}

	if ( i > 1024 )
		return NULL;

	pDownlaod->pURL = (char *) malloc( 1024 );
	memset( pDownlaod->pURL, 0, 1024 );

	// 반복문에서 구한 길이 만큼 복사한다.
	strncpy( pDownlaod->pURL, pBuf, i );
	// ------------ GUID Value Parsing End

	pBuf += (Download_URL_OFFSET - URL_URL_OFFSET);		// 0x1D4 - 0x68 = 0x16c
	//printf("pBuf의 첫 글자 (오프셋 확인용) = %x\n\n", *pBuf) ;   // 0x68 이 나온다면 offset은 정확한 위치임.

	//printf( "pbuf[%d] = %x\n", j, pBuf[j]);		// 0x68
	//printf( "pbuf[%d] = %x\n", j+1, pBuf[j+1]);	// 0x00
	//printf( "pbuf[%d] = %x\n", j+2, pBuf[j+2]);	// 0x74
	//printf( "pbuf[%d] = %x\n", j+3, pBuf[j+3]);	// 0x00

	while (	pBuf[j] != 0x00 || pBuf[j+1] != 0x00 )
	{
		//printf( "pbuf[%d] = %x\n", j, pBuf[j]);
		j++;
	}

	if ( j > 2048 )
		return NULL;

	//printf(" \nString의 길이 = %d\n", j );
	//j = j / 2 ;

	pDownlaod->pReferer = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pReferer, 0, 2048 );
	//wcsncpy( pDownlaod->pReferer, (WCHAR *)pBuf , j );
	memcpy ( pDownlaod->pReferer, pBuf , j+1);

	char *Temp0;
	int len;
	len = WideCharToMultiByte(CP_ACP, 0, pDownlaod->pReferer, -1, NULL, 0, NULL,NULL);
	Temp0 = new char[len];
	WideCharToMultiByte(CP_ACP, 0, pDownlaod->pReferer, -1, Temp0, 128, NULL, NULL);

	//wprintf(L"\n\n DEBUG pReferer = %s\n", pDownlaod->pReferer);
	//printf(" Convert = %s", Temp0);

	/* Hex dump
		printf("================== HEX DUMP START ===============\n");
		printf("0x%08x   ", &pDownlaod->pReferer);

		for ( a = 0; a < j ; a++)
			printf("%02x", pDownlaod->pReferer[a]);

		printf("\n================== HEX DUMP END ================\n");
	*/


	pBuf += j+3; // DownloadURL로 포인터 이동.
	//printf("pBuf의 두 글자 (오프셋 확인용) = %x\n\n", *pBuf) ;   // 0x68 이 나온다면 offset은 정확한 위치임.

	while (	pBuf[k] != 0x00 || pBuf[k+1] != 0x00 )
	{
		k++;
	}

	if ( k > 2048 )
		return NULL;

	pDownlaod->pDownloadURL = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pDownloadURL, 0, 2048 );
	//wcsncpy( pDownlaod->pDownloadURL, (WCHAR *)pBuf , j );
	memcpy ( pDownlaod->pDownloadURL, pBuf , k+1);

	char *Temp1;
	int len1;
	len1 = WideCharToMultiByte(CP_ACP, 0, pDownlaod->pDownloadURL, -1, NULL, 0, NULL,NULL);
	Temp1 = new char[len1];

	WideCharToMultiByte(CP_ACP, 0, pDownlaod->pDownloadURL, -1, Temp1, 128, NULL, NULL);

	//wprintf(L"\n\n DEBUG pDownloadURL = %s\n", pDownlaod->pDownloadURL);
	//printf(" Convert = %s", Temp1);

	pBuf += k+3; // Location으로 포인터 이동.
	//printf("pBuf의 세 글자 (오프셋 확인용) = %x\n\n", *pBuf) ;   // 0x68 이 나온다면 offset은 정확한 위치임.

	while (	pBuf[l] != 0x00 || pBuf[l+1] != 0x00 )
	{
		l++;
	}

	if ( l > 2048 )
		return NULL;

	pDownlaod->pLocation = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pLocation, 0, 2048 );
	//wcsncpy( pDownlaod->pLocation, (WCHAR *)pBuf , j );
	memcpy ( pDownlaod->pLocation, pBuf , l+1);

	char *Temp2;
	int len2;
	len2 = WideCharToMultiByte(CP_ACP, 0, pDownlaod->pLocation, -1, NULL, 0, NULL,NULL);
	Temp2 = new char[len2];

	WideCharToMultiByte(CP_ACP, 0, pDownlaod->pLocation, -1, Temp2, 128, NULL, NULL);

	//wprintf(L"\n\n DEBUG pLocation = %s\n", pDownlaod->pLocation);
	//printf(" Convert = %s\n", Temp2);

	//printf(" ############################# function END \n");

	return pDownlaod;	
}

struct history *getURL( char *pBuf, int nType ) {

	int i = 0;
	struct history *pHistory;
	FILETIME ft;

	if ( nType == TYPE_URL ) {

		pHistory = (struct history*) malloc ( sizeof( struct history ) );

		// Last accessed Time Stamp 구조체에 저장.
		memcpy( (DWORD *)&ft.dwLowDateTime, pBuf + URL_TIME_OFFSET, sizeof( DWORD ) );
		memcpy( (DWORD *)&ft.dwHighDateTime, pBuf + URL_TIME_OFFSET + 4, sizeof( DWORD ) );

		//FileTimeToSystemTime( &ft, &pHistory->st );
		pHistory->st = ft;
		pHistory->nType = TYPE_URL;

		pBuf += URL_URL_OFFSET;

	}

	// REDR 속성 파일 일단 no-Touch
	else if ( nType == TYPE_REDR ) {
		pHistory = (struct history*) malloc ( sizeof( struct history ) );
		ft.dwHighDateTime = 0;
		ft.dwLowDateTime = 0;
		//FileTimeToSystemTime( &ft, &pHistory->st );
		pHistory->st = ft;
		pHistory->nType = TYPE_REDR;
		pBuf += REDR_URL_OFFSET;
	}

	else {
		return NULL;
	}

	// 현재 위치 (실제 URL이 저장된 위치에서 부터 NULL이 나오는 부분까지의 길이를 구하기 위해서 i를 증가함)
	while ( pBuf[i] != 0 ) {  
		i++;
	}

	if ( i > 1024 )
		return NULL;

	pHistory->pURL = (char *) malloc( 1024 );
	if(pHistory->pURL ==NULL){
			puts("pHistory->pURL Malloc Failed...");
			exit(1);
	}
	memset( pHistory->pURL, 0, 1024 );

	// 반복문에서 구한 길이 만큼 복사한다.
	strncpy( pHistory->pURL, pBuf, i );

	return pHistory;	
}

void print_DownloadHistory( struct history_download *pDownlaod ) {

	char bufType[256];

	memset( bufType, 0, sizeof( bufType ) );
	if ( pDownlaod->nType == TYPE_URL ) {
		strcpy( bufType, "URL");
	}
	else if ( pDownlaod->nType == TYPE_REDR ) {
		strcpy( bufType, "REDR");
	}
	
	
	fprintf(stdout, "%s|", bufType );   // URL or REDR
	/*
		// The REDRs do not have a time stamp I think ..... 
	if ( pDownlaod->nType != TYPE_REDR )
		fprintf(stdout, "%d/%d/%d %d:%d:%d|", pDownlaod->st.wYear, pDownlaod->st.wMonth, pDownlaod->st.wDay, pDownlaod->st.wHour, pDownlaod->st.wMinute, pDownlaod->st.wSecond);
	else
		// skip date and time 
		fprintf(stdout, " |");	// 행 구분 문자
	*/

	fprintf(stdout, "%s|", pDownlaod->pURL );  // URL 파싱.
	fprintf(stdout, "%S|", pDownlaod->pReferer );
	fprintf(stdout, "%S|", pDownlaod->pDownloadURL );
	fprintf(stdout, "%S|\n", pDownlaod->pLocation );
}

void printHistory( struct history *pHistory ) {

	char bufType[256];

	memset( bufType, 0, sizeof( bufType ) );
	if ( pHistory->nType == TYPE_URL ) {
		strcpy( bufType, "URL");
	}
	else if ( pHistory->nType == TYPE_REDR ) {
		strcpy( bufType, "REDR");
	}
	
	
	fprintf(stdout, "%s|", bufType );   // URL or REDR
	
	/* 
		// The REDRs do not have a time stamp I think ..... 
	if ( pHistory->nType != TYPE_REDR )
		fprintf(stdout, "%d/%d/%d %d:%d:%d|", pHistory->st.wYear, pHistory->st.wMonth, pHistory->st.wDay, pHistory->st.wHour, pHistory->st.wMinute, pHistory->st.wSecond);
	else
	    // skip date and time 
		fprintf(stdout, " |");	// 행 구분 문자
	*/

	fprintf(stdout, "%s\n", pHistory->pURL );  // URL 파싱.

}

int main(int argc, char **argv) 
{
	//setlocale(LC_ALL, "korean");
	//_wsetlocale(LC_ALL, L"korean");

	FILE *pFD = NULL;
	char *pBuf = NULL;
	long lFileSize, lRead;
	long i = 0;

	int mode = 2; // 동작모드 설정 1 = cookie, history, cache 파싱 / 2 = download 리스트 파싱.
	
	struct history *pHistory;
	struct history_saved *pHistory_saved;

	struct history_download *pDownload;
	struct history_download_saved *pDownload_saved;
	
	int nType = 0;
	DWORD dwURLCount = 0;

	pFD = fopen(argv[1], "rb");

	if ( pFD == NULL ) {
		fprintf(stderr, "ERROR: File \"%s\" not found\n", argv[1]);
		exit(1);
	}

	fseek( pFD, 0, SEEK_END ); // 파일의 마지막 위치 기준  http://luckyyowu.tistory.com/21
	lFileSize = ftell( pFD );	// 현재 위치한 위치 값을 반환 한다.  -- 마지막 위치 이므로 결과적으로 파일의 크기를 의미한다.
	fseek( pFD, 0, SEEK_SET ); // 파일의 첫 번째 위치 기준

	pBuf = (char *)malloc( lFileSize );  // 파일 크기 만큼의 동적메모리 자동 할당.

	if ( pBuf == NULL ) {
		fprintf(stderr, "Failed to allocate memory");
		exit(1);
	}

	lRead = fread( pBuf, lFileSize, 1, pFD );

	if ( lRead != 1 ) {
		fprintf(stderr, "WARNING: Failed to read complete file\n");
	}

	
	// 구조체 동적 할당을 위한 전체 엔트리 갯수 구함
	while ( i < lFileSize )
	{
		if ( ( nType = bMatchPattern( pBuf + i ) ) > 0 ) 
		{
				dwURLCount ++;		
		}
		i++;
	}
	
	//printf("@@@@ DEBUG 전체 entry 갯 수 = %d\n", dwURLCount);
	
	i = 0;
	nType = 0;

	if (mode == 1)
	{
		pHistory_saved = (struct history_saved *)calloc(dwURLCount, sizeof(struct history_saved));
		if(pHistory_saved ==NULL){
			 puts("pHistory_Saved Malloc Failed...");
			 exit(1);
		}
	}

	if (mode == 2)
	{
		pDownload_saved = (struct history_download_saved *) calloc (dwURLCount, sizeof(struct history_download_saved));
		if(pDownload_saved ==NULL){
			 puts("pDownload_Saved Malloc Failed...");
			 exit(1);
		}
	}
	

	unsigned __int64 _timestamp;
	int z = 0;
	
	while ( i<lFileSize ) {   // 0 ~ 파일 전체 크기 동안 반복. (1바이트씩 쭊 반복해서 파싱하는 구조)

		if ( ( nType = bMatchPattern( pBuf + i ) ) > 0 )   // TYPE URL 혹은 REDR이 나온 경우에만 아래 구문 실행.
		{

			// Cookie, Cache, History   index.dat Parsing Part
			if ( mode == 1)
			{
				pHistory = getURL( pBuf + i, nType );

				
				pHistory_saved[z].nType = (int)pHistory->nType;
				
				FileTimeToUnixTime(&pDownload->st, &_timestamp);
				pHistory_saved[z].epoch = _timestamp;

				strncpy(pHistory_saved[z].pURL, pHistory->pURL, 1024);
				
				//printf ( "entry = %d\n", z );
				//printf ( "test 1 %d\n", pHistory_saved[z].nType );
				//printf ( "test 2 %s\n", pHistory_saved[z].pURL );
				//printf ( "test 2 %ld\n\n", pHistory_saved[z].st );
				
				z ++;
				
				if ( pHistory ) 
				{
					//printHistory( pHistory );
					free( pHistory->pURL );
					free( pHistory );
				}
			}

			// DownloadList  index.dat Parsing Part			
			if ( mode == 2)
			{
				pDownload = getDownload( pBuf +i, nType );

				pDownload_saved[z].nType = (int)pDownload->nType;

				FileTimeToUnixTime(&pDownload->st, &_timestamp);
				pDownload_saved[z].epoch = _timestamp;

				strncpy(pDownload_saved[z].pURL, pDownload->pURL, 1024);

				char *Referer;
				int len1;
				len1 = WideCharToMultiByte(CP_ACP, 0, pDownload->pReferer, -1, NULL, 0, NULL,NULL);
				Referer = new char[len1];
				WideCharToMultiByte(CP_ACP, 0, pDownload->pReferer, -1, Referer, 128, NULL, NULL);

				strncpy(pDownload_saved[z].pReferer, Referer, 1024);

				char *DownloadURL;
				int len2;
				len2 = WideCharToMultiByte(CP_ACP, 0, pDownload->pDownloadURL, -1, NULL, 0, NULL,NULL);
				DownloadURL = new char[len2];
				WideCharToMultiByte(CP_ACP, 0, pDownload->pDownloadURL, -1, DownloadURL, 128, NULL, NULL);

				strncpy(pDownload_saved[z].pDownloadURL, DownloadURL, 1024);

				char *Location;
				int len3;
				len3 = WideCharToMultiByte(CP_ACP, 0, pDownload->pLocation, -1, NULL, 0, NULL,NULL);
				Location = new char[len3];
				WideCharToMultiByte(CP_ACP, 0, pDownload->pLocation, -1, Location, 128, NULL, NULL);

				strncpy(pDownload_saved[z].pLocation, Location, 1024);
			
				z++;

				if ( pDownload ) 
				{
					//print_DownloadHistory( pDownload );
					free( pDownload->pURL );
					free( pDownload->pReferer );
					free( pDownload->pDownloadURL );
					free( pDownload->pLocation );
					free (pDownload );
				}
			}

		}
		
		i ++;
	}

	printf ( " LOOP 탈출 \n");

	fclose( pFD );
	free( pBuf );


	fprintf(stderr, "Urls retrieved %d\n", dwURLCount);

	if (mode == 1)
	{
		for ( z=0; z<dwURLCount; z++)
		{
			printf(" 들어간 값 확인1 : %d\n", pHistory_saved[z].nType);
			printf(" 들어간 값 확인2 : %I64d\n", pHistory_saved[z].epoch);
			printf(" 들어간 값 확인3 : %s\n", pHistory_saved[z].pURL);
		}
	}

	if (mode == 2)
	{
		for ( z=0; z<dwURLCount; z++)
		{
			printf(" 들어간 값 확인1 : %d\n", pDownload_saved[z].nType);
			printf(" 들어간 값 확인2 : %I64d\n", pDownload_saved[z].epoch);
			printf(" 들어간 값 확인3 : %s\n", pDownload_saved[z].pDownloadURL);
			printf(" 들어간 값 확인4 : %s\n", pDownload_saved[z].pReferer);
			printf(" 들어간 값 확인5 : %s\n", pDownload_saved[z].pLocation);
			printf(" 들어간 값 확인6 : %s\n", pDownload_saved[z].pURL);
		}
	}


	
	if (mode == 1)
	{
			// DB에 넣는 코드 작성!!!!! 
	}

	if (mode == 2)
	{
			// DB넣는 코드 작성 !!!
	}

	if ( mode == 1 )
		free (pHistory_saved);

	if (mode == 2)
		free (pDownload_saved);

}