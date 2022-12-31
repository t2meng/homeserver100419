/*
 * CPrettyText 数据库处理类 0.01
 * 
 * Author: Huang Guan
 * Email: huang_2008@msn.com
 * QQ: 357339036
 * Date: 2006-7-22
 * Description:
 * 这是一个很有趣的数据操作类，读写的文件是类似XML的文本存储方式，
 * 虽说是类似，但大有不同。使用起来十分简单，方便，详细见帮助。
 * 在实现方面，用了家族链结构（这是什么啊？我自创的:-D），就是每
 * 个节点（代码里有时叫元素）都父亲节点，孩子节点，兄长节点和小弟
 * 节点，这有点像树。不过的确是一棵树，只不过可以以族谱的形式来罗
 * 列这些数据结构。这种结构我曾经在开发OS的时候用作文件系统，不错
 * 的。节约资源。
 * 估计PrettyText还不是很完善，如果发现Bug，请马上通知我，我会修
 * 正错误，并且在新版本能够有更好的支持！
 * PrettyText是免费的数据操作类，代码公开，你可以任意修改，增加新
 * 功能，构思自己的数据库处理方式。但若要将此代码用在商业用途上，
 * 必须先联系我。
 *
 * 好了，尽情地享用吧！PrettyText会带给你新享受！
 *
 *
 */

#ifndef CPRETTYTEXT_H
#define CPRETTYTEXT_H

#include <string>
#include <vector>
using namespace std;

enum PRETTYERROR{
    ERR_OK,
    ERR_OPENFILE = 100,
    ERR_READFILE,
	ERR_FORMAT,
    ERR_UNKNOWN,
};
/*
 * CPrettyText 定义
 */
class CPrettyText
{
    private:
        /*
         * 注意：  下面是关于数据库文件的信息描述
         */
        time_t tmModified;  //修改时间
        size_t szFileSize;  //文件大小
        string strFileName; //文件名称，文件路径
        char *pBuffer;   //文件内容缓冲区
         
        //========================================================================== 
        /*
         * 注意： 下面是关于数据库读入内存中的存储格式
         * 目的是便于搜索。
         * 到时候可能会增加一个属性选项，不过现在版本还没有。
         * 例如 <Name type="String"> Flash </Name>
         */ 
        // 定义内存中存储单元结构
        struct MEM_UNIT{
            int iParent;  //父亲 
            int iChild;   //孩子 
            int iLeft;  //兄 
            int iRight; //弟 
            string strName;     //名称
            string strValue;    //内容 
        };
        // 带深度的内存单元结构 
        struct TagStack{
            int iLevel; //深度 
            int iNum;   //内存单元结构中的序号 
            string strText; //用来读取数据吧 
            MEM_UNIT unit;
        };
        // 添加单元结构
        // vUnit.push_back( newUnit );
        vector<MEM_UNIT> vUnit; //存储单元结构的向量 
        int iHead;  //第一个存储单元入口 
        int iCurrent;	//当前所选择的存储单元
	public:
		string& operator [](const string strName);
		void FormatText(string& strText, int M);
		// 查找名为strName的记录
		int Find(string strName);
		int Append(string strName);	//追加一个单元
		int Delete();
		int IsBegin();
		int IsEnd();
		int Dump();
		string& Name();
		int Child();
		int Parent();
		int Next();
		int Previous();
		int Delete(string strDir);
		string& Value();
		int Save();
		// class constructor
		CPrettyText();
		// class constructor with opening the file
		CPrettyText(string strFileName);
		// class destructor
		~CPrettyText();
		/**
		 * 打开数据库文件
		 * 并且以广义表形式存入内存中
		 * 2006-7-21
		 */
		int Open(string strFileName);
		// 获得strDir路径下的内容 
		string& Contents(string strDir);
		// 查找当前路径下strName字段的值＝＝strValue的记录
		int FindFirst(string strName, string strValue);
		// 从当前记录开始查找另一个strName字段的值＝＝strValue的记录
		int FindNext(string strName, string strValue);
		// 从当前目录再进入其中
		int Enter(string strDir);
		// 删除表中所有元素
		int Clear();
		// 访问所有元素，用于调试, iNum=0时，访问所有 
		int Visit(int iNum);
		// No description
		int Last();
		//排序 
		void SortBy( string strName, int order );
		string getValue( int iNum, string& strName );
	private:
		int AddRightTag(int iNum, string strName);
		int AddChildTag(int iNum, string strName);
		int Trim();
		int FreeTag(int iNum);
		int DeleteChildTag(int iNum);
		int EnterTag(string strName);
		int GoHome();
		// for Save();
		int SaveTag(FILE*fp, int iNum);
};

namespace PString{
    void Replace(string & strBig, const string & strsrc, const string &strdst);
	string ToString(int iNum);
	string ToLower(const string& str);
	string ToUpper(const string& str);
}

#endif // CPRETTYTEXT_H
