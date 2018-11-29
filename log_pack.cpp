#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <algorithm>
#include <cmath>
#include <ctgmath>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace std;

#define		LOGDIR 		"/home/log/"
#define		LOGHEADER	"klog"
#define		TXTKEY		"txt"
#define		TARKEY		"tar"
#define		TXTTYPE		".txt"
#define		TARTYPE		".tar.gz"
#define		DEFTLOG		LOGDIR LOGHEADER TXTTYPE
#define		DEFTTAR		LOGDIR LOGHEADER TARTYPE
#define		KLOG		LOGDIR LOGHEADER

//#define		DEBUG

void getNum(string &str, string &num)
{
	string n;
	size_t pos1 = str.find("g");
	size_t pos2 = str.find(".");

	if ( pos1 != string::npos  && pos2 != string::npos )
		n = str.substr(pos1 + 1, pos2 - pos1 - 1);

	num = n;
}

string getMaxNum(const string &a, const string &b)
{
	if ( a.length() < b.length() )
		return b;

	if ( a.length() == b.length() && a < b )
		return b;

	return a;
}

int main(int argc, char *argv[])
{
	char *p;
	char s[8];
	int ret = 0;
	int pack = 50;
	int clean = 40;
	int max_space = 80;	//MB
	int fileCnt = 0;
	long int m_Bytes = 0;
	double file_to_clean = 0;
	DIR *stream;
	struct dirent *dp;
	struct stat st;
	string str;
	string num;
	string txtMaxNum;
	string tarMaxNum;
	string txtFileName(KLOG);
	string tarFileName(KLOG);


	if (argc != 4) {
		printf("\n%s <pack> <clean> <max_space>\
				\n--pack		a number represent of how many log file triger tar\
				\n--clean		an approximately number represent of how many percentage of space should be clean when space full\
				\n--max_space	a number represent of log directory space size unit(MB)\
				\n\ndefault:\
				\n	pack=50\
				\n	clean=40\
				\n	max_space=80\
				\n	50 log file triger tar\
				\n	80MB space for log save\
				\n	if space full, clean about 40%%\
				\n\n", argv[0]);
		exit(0);
	}

	pack = atoi(argv[1]);
	clean = atoi(argv[2]);
	max_space = atoi(argv[3]);

	stream = opendir(LOGDIR);

	/* get log dir capacity */
	while((dp = readdir(stream)) != NULL)
	{
		string filepath(LOGDIR);

		if(dp->d_name[0] == '.')	continue;

		filepath.append(dp->d_name);

		ret = stat(filepath.c_str(), &st);
		if ( ret < 0 ) {
			printf("stat error %d\n", ret);
			exit(0);
		}

		m_Bytes += st.st_size;
		fileCnt++;
	}

#ifdef DEBUG
	printf("file_cnt:%d m_bytes:%ld\n", fileCnt, m_Bytes);
#endif

	/* space clean */
	if (( m_Bytes >> 20 ) >= max_space)
	{
		int base = 100;
		fileCnt < 100 ? base = 10 : base = 100;
		file_to_clean = round(fileCnt * clean / base);
#ifdef DEBUG
		printf("file_to_clean:%f\n", file_to_clean);
#endif

		seekdir(stream, 0);
		while(file_to_clean && (dp = readdir(stream)) != NULL)
		{
			static int cnt = 0;

			if(dp->d_name[0] == '.')	continue;

			str = dp->d_name;

			if (cnt == file_to_clean)	break;

			if ( str.find(TARKEY) != string::npos ) {
				getNum(str, num);
				if (atoi(num.c_str()) <= file_to_clean) {
					string rm_cmd("rm " LOGDIR);
					rm_cmd.append(str);
#ifdef DEBUG
					cout << rm_cmd << endl;
#endif
					system(rm_cmd.c_str());
					cnt++;
				}
			}
		}
	}

	/* 
	  file arrange
	  make tar file in order after clean
	 */
	if (file_to_clean) {
		seekdir(stream, 0);
		system("mkdir -p " LOGDIR "tmp");
		while((dp = readdir(stream)) != NULL)
		{
			if(dp->d_name[0] == '.')	continue;

			str = dp->d_name;

			if ( str.find(TARKEY) != string::npos )
			{
				string mv_cmd("mv " LOGDIR);
				mv_cmd.append(str);

				string dstfile(" " LOGDIR "tmp/" LOGHEADER);
				getNum(str, num);
				snprintf(s, sizeof(s), "%d", (int)(atoi(num.c_str()) - file_to_clean));
				num = s;
				dstfile.append(num);
				dstfile.append(TARTYPE);
				mv_cmd.append(dstfile);
#ifdef DEBUG
				cout << mv_cmd << endl;
#endif
				system(mv_cmd.c_str());
			}
		}
		system("mv " LOGDIR "tmp/* " LOGDIR);
		system("rmdir " LOGDIR "tmp");
	}

	/* find max num of tar and txt file */
	seekdir(stream, 0);
	while((dp = readdir(stream)) != NULL)
	{
		if(dp->d_name[0] == '.')	continue;

#ifdef DEBUG
		printf("%s   #\n", dp->d_name);
#endif

		str = dp->d_name;

		if ( str.find(TXTKEY) != string::npos ) {
			getNum(str, num);
			txtMaxNum = getMaxNum(num, txtMaxNum);
		} else if ( str.find(TARKEY) != string::npos ) {
			getNum(str, num);
			tarMaxNum = getMaxNum(num, tarMaxNum);
		}
	}

#ifdef DEBUG
	cout << "max txt num is " << txtMaxNum << endl;
	cout << "max tar num is " << tarMaxNum << endl;
#endif

	/* rename */
	snprintf(s, sizeof(s), "%d", atoi(txtMaxNum.c_str()) + 1);
	txtMaxNum = s;

	snprintf(s, sizeof(s), "%d", atoi(tarMaxNum.c_str()) + 1);
	tarMaxNum = s;

	txtFileName.append(txtMaxNum);
	txtFileName.append(TXTTYPE);

	tarFileName.append(tarMaxNum);
	tarFileName.append(TARTYPE);

#ifdef DEBUG
	cout << "next txt name is " << txtFileName << endl;
	cout << "next tar name is " << tarFileName << endl;
#endif

	rename(DEFTLOG, txtFileName.c_str());

	/* package */
	if ( atoi(txtMaxNum.c_str()) >= pack ) {
		system("mkdir -p " KLOG);
		system("mv " KLOG "*" TXTTYPE " " KLOG);
		system("tar zcvf " DEFTTAR " " KLOG);
		system("rm -rf " KLOG);
		rename(DEFTTAR, tarFileName.c_str());
	}

	closedir(stream);

	return 0;
}
