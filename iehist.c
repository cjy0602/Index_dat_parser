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

//setlocale(LC_ALL, "korean");

const int TYPE_URL  = 0x01;
const int TYPE_REDR = 0x02;

const int URL_URL_OFFSET  = 104;  // 0x68 Next, we see that the actual URL the user visited is located at offset 0x68 from the beginning o
const int URL_TIME_OFFSET = 16;  // Last accessed Time Stamp
const int REDR_URL_OFFSET = 16;  

const int Download_URL_OFFSET = 468; // 0x1D4;

struct history {
	int nType;
	char *pURL;
	SYSTEMTIME st;
	//SYSTEMTIME st2;
};

// IEHistoryDownload - index.dat Parsing Struct 
struct history_download {
	int nType;
	char *pURL;
	SYSTEMTIME st;
	WCHAR *pReferer;
	WCHAR *pDownloadURL;
	WCHAR *pLocation;
};


int bMatchPattern( char *pBuf ) {

	// 
	//if ( pBuf[0] == 0x55 && pBuf[1] == 0x52 && pBuf[2] == 0x4c && pBuf[3] == 0x20 && pBuf[5] == 0x00 && pBuf[6] == 0x00)
	if ( pBuf[0] == 0x55 && pBuf[1] == 0x52 && pBuf[2] == 0x4c && pBuf[3] == 0x20)
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

	struct history_download *pDownlaod;
	FILETIME ft;

	if ( nType == TYPE_URL ) {

		pDownlaod = (struct history_download*) malloc ( sizeof( struct history_download ) );

		// Last accessed Time Stamp 구조체에 저장.
		memcpy( (DWORD *)&ft.dwLowDateTime, pBuf + URL_TIME_OFFSET, sizeof( DWORD ) );
		memcpy( (DWORD *)&ft.dwHighDateTime, pBuf + URL_TIME_OFFSET + 4, sizeof( DWORD ) );
		                                 
		FileTimeToSystemTime( &ft, &pDownlaod->st );
		pDownlaod->nType = TYPE_URL;

		pBuf += URL_URL_OFFSET;

	}
	// REDR 속성 파일 일단 no-Touch
	else if ( nType == TYPE_REDR ) {
		pDownlaod = (struct history_download*) malloc ( sizeof( struct history_download ) );
		ft.dwHighDateTime = 0;
		ft.dwLowDateTime = 0;
		FileTimeToSystemTime( &ft, &pDownlaod->st );
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

	while (	pBuf[j] != 0x00 && pBuf[j+1] != 0x00 )
	{
		j++;
	}

	if ( j > 2048 )
		return NULL;

	pDownlaod->pReferer = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pReferer, 0, 2048 );
	wcsncpy( pDownlaod->pReferer, (WCHAR *)pBuf , j );
	//memcpy( pDownlaod->pReferer, (WCHAR *)pBuf, j );

	printf("\n\n DEBUG pReferer = %S\n\n", pDownlaod->pReferer);

	pBuf += j+2; // DownloadURL로 포인터 이동.

	while (	pBuf[k] != 0x00 && pBuf[k+1] != 0x00 )
	{
		k++;
	}

	if ( k > 2048 )
		return NULL;

	pDownlaod->pDownloadURL = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pDownloadURL, 0, 2048 );
	wcsncpy( pDownlaod->pDownloadURL, (WCHAR *)pBuf , j );

	pBuf += k+2; // Location으로 포인터 이동.

	while (	pBuf[l] != 0x00 && pBuf[l+1] != 0x00 )
	{
		l++;
	}

	if ( l > 2048 )
		return NULL;

	pDownlaod->pLocation = (WCHAR *) malloc( 2048 );
	memset( pDownlaod->pLocation, 0, 2048 );
	wcsncpy( pDownlaod->pLocation, (WCHAR *)pBuf , j );

	return pDownlaod;	
}

struct history *getURL( char *pBuf, int nType ) {

	int i = 0;
	struct history *pHistory;
	FILETIME ft;

	if ( nType == TYPE_URL ) {

		pHistory = (struct history*) malloc ( sizeof( struct history ) );

		// Fri Nov 22 1963 00:00:00 형식을 원한다. 
		// Last accessed Time Stamp 구조체에 저장.
		memcpy( (DWORD *)&ft.dwLowDateTime, pBuf + URL_TIME_OFFSET, sizeof( DWORD ) );
		memcpy( (DWORD *)&ft.dwHighDateTime, pBuf + URL_TIME_OFFSET + 4, sizeof( DWORD ) );

		FileTimeToSystemTime( &ft, &pHistory->st );
		pHistory->nType = TYPE_URL;

		pBuf += URL_URL_OFFSET;

	}

	// REDR 속성 파일 일단 no-Touch
	else if ( nType == TYPE_REDR ) {
		pHistory = (struct history*) malloc ( sizeof( struct history ) );
		ft.dwHighDateTime = 0;
		ft.dwLowDateTime = 0;
		FileTimeToSystemTime( &ft, &pHistory->st );
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
	
	/* The REDRs do not have a time stamp I think ..... */
	if ( pDownlaod->nType != TYPE_REDR )
		fprintf(stdout, "%d/%d/%d %d:%d:%d|", pDownlaod->st.wYear, pDownlaod->st.wMonth, pDownlaod->st.wDay, pDownlaod->st.wHour, pDownlaod->st.wMinute, pDownlaod->st.wSecond);
	else
		/* skip date and time */
		fprintf(stdout, " |");	// 행 구분 문자

	fprintf(stdout, "%s|\n", pDownlaod->pURL );  // URL 파싱.
	fprintf(stdout, "%S|\n", pDownlaod->pReferer );
	fprintf(stdout, "%S|\n", pDownlaod->pDownloadURL );
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
	
	/* The REDRs do not have a time stamp I think ..... */
	if ( pHistory->nType != TYPE_REDR )
		fprintf(stdout, "%d/%d/%d %d:%d:%d|", pHistory->st.wYear, pHistory->st.wMonth, pHistory->st.wDay, pHistory->st.wHour, pHistory->st.wMinute, pHistory->st.wSecond);
	else
		/* skip date and time */
		fprintf(stdout, " |");	// 행 구분 문자
	
	fprintf(stdout, "%s\n", pHistory->pURL );  // URL 파싱.

}

void usage(char **argv) {
	
	fprintf(stderr, "\nInternet History Viewer v.0.0.1 by patrik@cqure.net\n");
	fprintf(stderr, "---------------------------------------------------\n");
	fprintf(stderr, "usage: %s <historyfile>\n", argv[0]);

}

int main(int argc, char **argv) 
{
	FILE *pFD = NULL;
	char *pBuf = NULL;
	long lFileSize, lRead;
	long i = 0;
	
	struct history *pHistory;
	struct history_download *pDownload;
	
	int nType = 0;
	DWORD dwURLCount = 0;

	if ( argc < 2 ) {
		usage(argv);
		exit(1);
	}

	if ( strncmp( argv[1], "-h", 2) == 0) {
		usage( argv );
		exit(1);
	}

	pFD = fopen(argv[1], "rb");

	if ( pFD == NULL ) {
		fprintf(stderr, "ERROR: File \"%s\" not found\n", argv[1]);
		exit(1);
	}

	fseek( pFD, 0, SEEK_END ); // 파일의 마지막 위치 기준  http://luckyyowu.tistory.com/21
	lFileSize = ftell( pFD );	// 현재 위치한 위치 값을 반환 한다.  -- 마지막 위치 이므로 결과적으로 파일의 크기를 의미한다.
	fseek( pFD, 0, SEEK_SET ); // 파일의 첫 번째 위치 기준

	pBuf = malloc( lFileSize );  // 파일 크기 만큼의 동적메모리 자동 할당.

	if ( pBuf == NULL ) {
		fprintf(stderr, "Failed to allocate memory");
		exit(1);
	}

	lRead = fread( pBuf, lFileSize, 1, pFD );

	if ( lRead != 1 ) {
		fprintf(stderr, "WARNING: Failed to read complete file\n");
	}

	while ( i<lFileSize ) {   // 0 ~ 파일 전체 크기 동안 반복. (1바이트씩 쭊 반복해서 파싱하는 구조)

		if ( ( nType = bMatchPattern( pBuf + i ) ) > 0 )   // TYPE URL 혹은 REDR이 나온 경우에만 아래 구문 실행.
		{
			//pHistory = getURL( pBuf + i, nType );
			pDownload = getDownload( pBuf +i, nType );

			/*
			if ( pHistory ) 
			{
				printHistory( pHistory );
				free( pHistory->pURL );
				free( pHistory );
				dwURLCount ++;
			}
			*/

			if ( pDownload ) 
			{
				print_DownloadHistory( pDownload );
				free( pDownload->pURL );
				free( pDownload->pReferer );
				free( pDownload->pDownloadURL );
				free( pDownload->pLocation );
				free (pDownload );
				dwURLCount ++;
			}

		}
		
		i ++;
	}


	fclose( pFD );
	free( pBuf );

	fprintf(stderr, "Urls retrieved %d\n", dwURLCount);

}