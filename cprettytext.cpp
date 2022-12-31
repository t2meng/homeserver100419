

#include "cprettytext.h" // class's header file
#include <iostream>
#include <io.h>
#include <stack>

#define P_DEBUG_INFO    //显示基本调试信息 
#define P_DEBUG_ERR     //显示错误信息
//#define P_DEBUG_READ

#ifdef P_DEBUG_INFO
#include <windows.h>	//for GetTickCount()
#endif
 
/*
 * 一些转义字符
 * <    &lt;        这个一定要转 *
 * >    &gt;        这个一定要转 *
 * '    &apos;      目前还可以不转 
 * &    &amp;       目前还可以不转 *
 * "    &quot;      目前还可以不转 
 * 换行 &rt;        这是自创的,可以不转 *
 * 空格 &nbsp;      可以不转 *
 */
#define TEXT_IN 0
#define TEXT_OUT 1
 
// class constructor
CPrettyText::CPrettyText()
{ 
	// insert your code here
	// cout<<sizeof(MEM_UNIT)<<endl;  24bits
	Clear();
}

// class constructor with openning the file
CPrettyText::CPrettyText(string strFileName)
{ 
	// insert your code here
	if(Open(strFileName)!=ERR_OK) {
		printf("Open file %s failed.", strFileName.c_str() );
    	Clear();
    }
}

// class destructor
CPrettyText::~CPrettyText()
{
	// insert your code here
}

/*
 * 打开数据库文件
 * 并且以广义表形式存入内存中
 * 如果文件已打开，如果文件没有做过更改，则不必重新读入 
 * 2006-7-21
 */
int CPrettyText::Open(string strFileName)
{
	/* TODO (#1#): Implement CPrettyText::Open() */
	/* 一个文件的结构大概如下，从PrettyText内开始读取*/
    /*
    <PrettyText>  
       <Teacher>
            <0001>
                <Name>Harry</Name>
                <Age>24</Age>
                <Teach>English</Teach>
            </0001> 
        </Teacher>
    </PrettyText>
    */
	/* 这是计算读取时间的，调试用 */
#ifdef P_DEBUG_INFO
	int ticks = GetTickCount();
#endif
    this -> strFileName = strFileName;
    /* 先打开文件读取 */ 
    FILE *fp = fopen( strFileName.c_str(), "rb" );
    if( fp == NULL )
    {
        return ERR_OPENFILE;
    }
    /* 下面获得文件大小 */ 
    szFileSize = filelength( fileno( fp ) );
    /* 申请缓冲区 */
    pBuffer = new char[szFileSize];  
    if( fread( pBuffer, szFileSize, 1, fp ) <=0 )
    {
        fclose( fp );
        return ERR_READFILE;
    }
    /* 关闭文件 */
    fclose( fp );   
    //OK! 现在我们可以开始读取数据库内容了
    enum Status{
        eInsideTag,     //在<>里面 
        eOutsideTag     //在<>外面 
    };
    enum TokenTypeTag{
        eTokenText,
        eTokenStart,        // <
        eTokenEnd,          // </
        eTokenClose,        // >
        eTokenEquals,       // =
        eTokenDeclaration,  // <?
        eTokenError
    };
    Status status = eOutsideTag; //状态标记
    TokenTypeTag token = eTokenText;
    stack<TagStack> stkTag;  //递归名称用堆栈装
    //int iLevel = 0; //这是表示第n层，术语上是深度 
    int iPos = 0, iLineNo = 1;
    bool bTagStart = true;
    string strText = "";    //临时变量 
    TagStack tagCurrent;	// 本来是={0,0,"",{-1,-1,-1,-1,"",""}};	//VC这里编译会出错。
	/* 对当前读取的元素初始化 */
	tagCurrent.iNum = 0;
	tagCurrent.iLevel = 0;
	tagCurrent.strText = "";
	tagCurrent.unit.iChild = -1;
	tagCurrent.unit.iParent = -1;
	tagCurrent.unit.iLeft = -1;
	tagCurrent.unit.iRight = -1;
	tagCurrent.unit.strName = "";
	tagCurrent.unit.strValue = "";
	vUnit.clear();	//清空内存所有元素内容

    /* 循环直到读完缓冲区 */ 
    while( iPos < szFileSize )
    {
        char ch = pBuffer[iPos++];    //读这个字符 
        // 辨别是什么字符 
        switch(ch)
        {
            case '\n':
            case '\r':
                iLineNo++; //行号增加 
            case ' ':
            case '\t':
		if( status == eOutsideTag && !tagCurrent.unit.strValue.empty() ){
			token = eTokenText;
			break;
		}else{
			continue;
		}
            case '<':
                if( iPos < szFileSize )
                {
                    if( pBuffer[iPos] == '/' )
                    {
                        token = eTokenEnd;
                        iPos++;
                    }
                    else if( pBuffer[iPos] == '?' )
                    {
                        token = eTokenDeclaration;
                        iPos++;
                    }
                    else
                    {
                        token = eTokenStart;
                    }
                }else{
                    token = eTokenStart;    //这是不可能的吧 
                }
                break;
            case '>':
                token = eTokenClose;
                break;
            default:
                token = eTokenText;
                break;
        }
#ifdef P_DEBUG_READ
        //printf("读：%c\n",ch);
#endif
        // 在尖括号内外 
        switch(status)
        {
            case eOutsideTag:
                switch(token)
                {
                    case eTokenStart:   // '<'
                        bTagStart = true;
                        status = eInsideTag;
                        /* 又有一个新的开始 */
                        if( tagCurrent.iLevel > 0 )
                        {
                            //进入下层，把上层的状态保存 
                            stkTag.push( tagCurrent );  
                            // tagCurrent.iLevel 不变
                            /* 初始化下列 */
                            tagCurrent.unit.strName = "";
                            tagCurrent.unit.strValue = "";
                            tagCurrent.unit.iParent = -1;
                            tagCurrent.unit.iChild = -1;
                            tagCurrent.unit.iLeft = -1;
                            tagCurrent.unit.iRight = -1;
                        }else{
                        } 
                        break;
                    case eTokenText:    //
                        if( tagCurrent.iLevel > 0 )
                        {
                            //strText = strText + ch;
                            tagCurrent.unit.strValue += ch;
                        }
                        break;
                    case eTokenEnd: // '</'
                        bTagStart = false;
                        status = eInsideTag;
                        tagCurrent.iLevel--;
#ifdef P_DEBUG_READ
                        printf("iLevel = %d\n",tagCurrent.iLevel);
#endif
                        break;
                    default:
                        /* ERROR */
#ifdef P_DEBUG_ERR
                        printf("行 %d：不可能出现的符号: %c\n",iLineNo,ch);
#endif
                        return ERR_UNKNOWN; 
                }
                break;
            case eInsideTag:
                switch(token)
                {
                    case eTokenText:
                        strText += ch;
                        break;
                    case eTokenClose:   // '>'
                        status = eOutsideTag;
                        if( bTagStart ) // "<..>"
                        {
							tagCurrent.unit.strName = strText;
                            strText = "";
                            tagCurrent.iNum = vUnit.size(); //记下编号
                            vUnit.push_back( tagCurrent.unit ); //先填充，将导致vUnit.size增1 
                            tagCurrent.iLevel++;
#ifdef P_DEBUG_READ
                            //printf("iLevel = %d\n",tagCurrent.iLevel);
#endif
                        }else{          // "</..>" 这里处理完成一个层的读取 
                            if( strText=="field"||tagCurrent.unit.strName == strText )
                            {
                                if ( stkTag.size() > 0 ) //如果有父亲或者兄弟 
                                { 
                                    TagStack *tag = &stkTag.top();   //取上层 
                                    if( tag->unit.iChild != -1 )  //上层已经是父亲, 所以它有兄弟
                                    {
										int i = tag->unit.iChild;
										while( vUnit[i].iRight != -1 )
											i = vUnit[i].iRight;
										vUnit[i].iRight = tagCurrent.iNum;
										tagCurrent.unit.iLeft = i;
                                    }
                                    else //不是父亲，现在变它为父亲 
                                    {
                                        tag->unit.iChild = tagCurrent.iNum;
										tagCurrent.unit.iParent = tag->iNum ;
                                    }
									FormatText(tagCurrent.unit.strName, TEXT_IN);
									FormatText(tagCurrent.unit.strValue, TEXT_IN);
                                    vUnit[ tagCurrent.iNum ] = tagCurrent.unit; //完成一个内容
                                    tagCurrent = stkTag.top();
                                    stkTag.pop(); 
                                }else{	/* 读取完毕 */
									FormatText(tagCurrent.unit.strName, TEXT_IN);
									FormatText(tagCurrent.unit.strValue, TEXT_IN);
                                    vUnit[ tagCurrent.iNum ] = tagCurrent.unit;

									/* 设置入口 */
                                    iHead = tagCurrent.iNum;    
									/* 当前元素 */
									iCurrent = iHead;
#ifdef P_DEBUG_INFO
                                    printf("Entrance: %d;    Number of units: %d\n", iHead, vUnit.size());
#endif
                                }
                            }else{
                                /* Error */
                                #ifdef P_DEBUG_ERR
                                printf("行 %d: 结束标记 %s 和开始标记 %s 不同。", iLineNo,
									tagCurrent.unit.strName.c_str(), strText.c_str());
                                #endif
                                return ERR_UNKNOWN; 
                            }
                            strText = "";
                        }
                        break;
                    default:
                        /* ERROR */
                        #ifdef P_DEBUG_ERR
                        printf("行 %d：不可能出现的符号: %c\n",iLineNo,ch);
                        #endif
                        return ERR_UNKNOWN; 
                        break;
                }
                break;
        }
    }
    //现在，我们已经成功地把所有数据都读进内存去了，并且以树形式存在。

	// 先做备份
    fp = fopen( (strFileName + ".bak").c_str(), "wb");
	if(fp!=NULL)
	{
		fwrite(pBuffer, szFileSize, 1, fp);
		fclose( fp );
	}
	//删除缓冲区
	delete pBuffer;

    //puts("All right."); 
#ifdef P_DEBUG_INFO
	printf("Read time: %dms\n", GetTickCount()-ticks);
#endif
	if( vUnit.size() < 1 )
	{
		Clear();
		return ERR_FORMAT;
	}
	return ERR_OK;
}


// 找到元素
// 注意，只有拥有子元素的才能进入
// Enter执行也不会失败，总是返回true
int CPrettyText::Enter(string strDir)
{
	/* TODO (#1#): Implement CPrettyText::Enter() */
	/*
	 * 我的想法是把路径分成一部分一部分的，例如
	 * /PrettyText/School/Student
	 * 可以写成
	 * PrettyText, School, Student 基部分，逐步进入
	 */
	//把\转换为/，且删除无效字符
	for( int i=0; i<strDir.length(); i++ )
	{
		switch( strDir[i] )
		{
		case '\\':
			strDir[i] = '/';
			break;
		case ' ':
		case '<':
		case '>':
		case '\r':
		case '\n':
			strDir.erase(i,1);
			i--;
			break;
		}
	}
	if( strDir[0]=='/' )
		GoHome(); //返回最顶层目录
	/* 删除前后的/ */
	while( strDir[0]=='/' ) strDir.erase( 0, 1 );
	while( strDir[strDir.length()-1]=='/' ) strDir.erase( strDir.length()-1, 1 );
	if(strDir.empty()) //如果是空的
		return true;
	//OK，现在拆分
	int iPos;
	while( (iPos = strDir.find('/'))!=-1 )	
	{
		//如果找到/，我们就分开两部分
		string strName = strDir.substr(0, iPos);	//注意，不要把/也放进去了:)
		strDir.erase(0,iPos+1);	//这里把/也删除
		if( strName.empty() )
			continue;
		if( !EnterTag( strName ) )
			return false;
	}
	if( !EnterTag( strDir ) )
		return false;
	return true;
}


string CPrettyText::getValue( int iNum, string& strName )
{
		int j = vUnit[iNum].iChild;
		while( j!=-1 )
		{
			if( strName == vUnit[j].strName )
			{
				return vUnit[j].strValue;
			}
			j = vUnit[j].iRight;
		}
		return "";
}

static CPrettyText* sort_pt;
static string sort_strName;	//当前排序名 
static int sort_iOrder;

static int sort_compare( const void *arg1, const void *arg2 )
{
	if( sort_iOrder == 0 ){
		return  atoi(sort_pt->getValue((*(int*)arg1), sort_strName).c_str()) - 
			atoi(sort_pt->getValue((*(int*)arg2), sort_strName).c_str());
	}else{
		return  atoi(sort_pt->getValue((*(int*)arg2), sort_strName).c_str()) - 
			atoi(sort_pt->getValue((*(int*)arg1), sort_strName).c_str());
	}
}

// 排序 
void CPrettyText::SortBy(string strName, int order)
{
	int i = vUnit[iCurrent].iChild;
	int count = 0;
	int * data, *p;
	sort_strName = strName;
	sort_iOrder = order;
	sort_pt = (CPrettyText*)this;
	//遍历所有兄弟	
	while( i != -1 )
	{
		count ++;
		i = vUnit[i].iRight;	//下一个兄弟
	}
	if( count == 0 )
		return ;
	data = new int[count];
	i = vUnit[iCurrent].iChild;
	vUnit[i].iParent = -1;
	p = data;
	while( i != -1 )
	{
		*p = i;
		p ++;
		i = vUnit[i].iRight;	//下一个兄弟
	}
	qsort( data, count, sizeof(int), sort_compare );
	p = data;
	vUnit[iCurrent].iChild = *p;
	vUnit[*p].iLeft = -1;
	vUnit[*p].iRight = *(p+1);
	vUnit[*p].iParent = iCurrent;
	count --;
	p++;
	while( count > 1 ){
		vUnit[*p].iLeft = *(p-1);
		vUnit[*p].iRight = *(p+1);
		p ++;
		count --;
	}
	vUnit[*p].iLeft = *(p-1);
	vUnit[*p].iRight = -1;
	delete[] data;
	return ;
}


// 查找当前路径下strName项的值＝＝strValue的记录
int CPrettyText::FindFirst(string strName, string strValue)
{
	/* TODO (#1#): Implement CPrettyText::Find() */
	strName = PString::ToLower(strName);
	int i = vUnit[iCurrent].iChild;
	//遍历所有兄弟	
	while( i != -1 )
	{
		int j = vUnit[i].iChild;
		while( j!=-1 )
		{
			if( strName == vUnit[j].strName && strValue == vUnit[j].strValue )
			{
				iCurrent = i;
				return true;
			}
			j = vUnit[j].iRight;
		}
		i = vUnit[i].iRight;	//下一个兄弟
	}
	return false;
}

// 另一个和当前名和值都相同的记录
int CPrettyText::FindNext(string strName, string strValue)
{
	/* TODO (#1#): Implement CPrettyText::Find() */
	strName = PString::ToLower(strName);
	int i = vUnit[iCurrent].iRight;	//There's a bug here before. 2008-12-6 15:25 Huang Guan
	//遍历所有兄弟	
	while( i != -1 )
	{
		int j = vUnit[i].iChild;
		while( j!=-1 )
		{
			if( strName == vUnit[j].strName && strValue == vUnit[j].strValue )
			{
				iCurrent = i;
				return true;
			}
			j = vUnit[j].iRight;
		}
		i = vUnit[i].iRight;	//下一个兄弟
	}
	return false;
}

// 获取元素的值
string& CPrettyText::Contents(string strDir)
{
	/* TODO (#1#): Implement CPrettyText::Contents() */
	/*
	 * 首先，必须知道，这个strDir可能是以下的情况
	 * A: /PrettyText/School/Teacher/0001/Name
	 * B: 0001/Name
	 * C: Name
	 * D: ..
	 * 当然，我想调用Enter，这样简单很多。
	 */
	int iOld = iCurrent;	//先保存当前元素，下面Enter后，会改变
	Enter( strDir );	//Enter是不可能失败的，我人格担保 :)
	//不知有什么好的办法不用第三方变量来调换iOld和iCurrent
	int iFound = iCurrent;
	iCurrent = iOld;	//返回原来元素
	return vUnit[iFound].strValue;
}

// 重载运算符
string& CPrettyText::operator [](const string strDir)
{
	int iOld = iCurrent;	//先保存当前元素，下面Enter后，会改变
	Enter( strDir );	//Enter是不可能失败的，我人格担保 :)
	//不知有什么好的办法不用第三方变量来调换iOld和iCurrent
	int iFound = iCurrent;
	iCurrent = iOld;	//返回原来元素
	return vUnit[iFound].strValue;
}

// 删除表中所有元素
int CPrettyText::Clear()
{
	/* TODO (#1#): Implement CPrettyText::Clear() */
	/* 清空所有 */
	vUnit.clear();
	iHead = 0;
	iCurrent = 0;
	/* 制作一个族长吧 */
	MEM_UNIT unit;
	unit.iParent = -1;
	unit.iLeft = -1;
	unit.iRight = -1;
	unit.iChild = -1;
	unit.strName = "PrettyText";
	unit.strValue = "";
	vUnit.push_back(unit);
	//cout<<"New"<<endl;
	return true;
}

/* 注意：此函数仅用于调试 */
// 打印某一元素下所拥有的所有子元素，用于调试 
int CPrettyText::Visit(int iNum)
{
	/* TODO (#1#): Implement CPrettyText::Visit() */
	static int iLv = 0;
	// 从 iNum 入口进入
	for( int i=0; i<iLv; i++ )
	   printf("    ");
	printf("%s: %s\n", vUnit[iNum].strName.c_str(), vUnit[iNum].strValue.c_str() );
	/* 如果有子，先访问子 */ 
    if( vUnit[iNum].iChild!=-1 )
    {
        iLv++;//进入一层 
        Visit( vUnit[iNum].iChild );
        iLv--;//退出一层 
    }
    /* 看看有没有弟 */
    if( vUnit[iNum].iRight!=-1 )
    {
        Visit( vUnit[iNum].iRight );
    } 
	return true;
}

/*
 * 从内存中保存到文件，可能需要花一段时间。
 */
int CPrettyText::Save()
{
	/* 这里按照一定格式写入文件 */
	// 首先打开文件
	FILE *fp = fopen(strFileName.c_str(), "wb");
	if( fp == NULL )
		return ERR_OPENFILE;
	// OK.我们开始安排格式
	/*
	 * 也理应是这样的：
	 <MyDataBase>
	     <School>
			...
		 </School>
	 </MyDataBase>
	 */
	// 这里我用递归来输出，否则会很麻烦。
	SaveTag(fp, 0);
	fclose(fp);
	return true;
}

int CPrettyText::SaveTag(FILE*fp, int iNum)
{
	/* 这里和Visit差不多 */
	static int iLv = 0;
	int i;
	//制表
	for( i=0; i<iLv; i++ )
		fputc('\t', fp);	
	// 如果没有孩子，但用简写 <Name>Flash</Name>
	string strName = vUnit[iNum].strName;
	string strValue = vUnit[iNum].strValue;
	FormatText(strName, TEXT_OUT);
	FormatText(strValue, TEXT_OUT);
	if( vUnit[iNum].iChild==-1 )
	{
		fprintf(fp, "<%s>%s</%s>\n", strName.c_str(), strValue.c_str(), strName.c_str() );
	}else{	//如果有子
	    fprintf(fp,"<%s>%s\n", strName.c_str(), strValue.c_str() );
		iLv++;
		SaveTag(fp, vUnit[iNum].iChild);
		iLv--;
		//制表
		for( i=0; i<iLv; i++ )
			fputc('\t', fp);	
		fprintf(fp,"</%s>\n", strName.c_str() );
	}
	if( vUnit[iNum].iRight != -1 )
	{
		SaveTag(fp, vUnit[iNum].iRight);
	}
	return true;
}

/* 返回顶层 */
int CPrettyText::GoHome()
{
	iCurrent = iHead;
	return true;
}

int CPrettyText::Append(string strName)
{
	return AddChildTag( iCurrent, strName);
}

/* 进入一层 */
int CPrettyText::EnterTag(string strName)
{
	//搜索，如果找到，就取其下标
	if( strName == "." )
		return true;
	//父亲
	if( strName == ".." )
	{	
		while( vUnit[iCurrent].iLeft != -1 )
			iCurrent = vUnit[iCurrent].iLeft;
		if( vUnit[iCurrent].iParent != -1 )
		{
			iCurrent = vUnit[iCurrent].iParent;
			return true;
		}else{
			return true;	//不做任何动作
		}
	}
	
	int i = vUnit[iCurrent].iChild;
	//遍历所有兄弟	
	while( i != -1 )
	{
		if( PString::ToLower(strName) == PString::ToLower(vUnit[i].strName) )
		{
			iCurrent = i;
			return true;
		}
		i = vUnit[i].iRight;	//下一个兄弟
	}
	//如果没有,则创建, 这里应该不会出错吧
	iCurrent = AddChildTag( iCurrent, strName );
	return true;
}

/* 在该元素下增加一个子元素 */
int CPrettyText::AddChildTag(int iNum, string strName)
{
	
	if( vUnit[iNum].iChild !=-1 )	//如果已经有孩子了，就增加孩子的兄弟
	{
		return AddRightTag( vUnit[iNum].iChild, strName );
	}else{
		//添加新子
		MEM_UNIT unit;
		unit.iParent = iNum;
		unit.iLeft = -1;
		unit.iRight = -1;
		unit.iChild = -1;
		unit.strName = strName;
		unit.strValue = "";
		vUnit[iNum].iChild = vUnit.size();
		vUnit.push_back(unit);
		return vUnit[iNum].iChild;
	}
}

/* 增加一个该元素的兄弟 */
int CPrettyText::AddRightTag(int iNum, string strName)
{
	while( vUnit[iNum].iRight != -1 )
		iNum = vUnit[iNum].iRight;
	//添加新兄弟
	MEM_UNIT unit;
	unit.iParent = -1;
	unit.iLeft = iNum;
	unit.iRight = -1;
	unit.iChild = -1;
	unit.strName = strName;
	unit.strValue = "";
	vUnit[iNum].iRight = vUnit.size();
	vUnit.push_back(unit);
	return vUnit[iNum].iRight;
}

/* 返回当前元素的值 */
string& CPrettyText::Value()
{
	return vUnit[iCurrent].strValue;
}


int CPrettyText::Delete()
{
	int iFound = iCurrent;
	if( iFound == iHead )	//族长不可以删除
		return false;
	/*
	 * 我们只是删除该元素的子，该元素的兄弟不会删除，
	 * 但该元素的子的兄弟又要删除，所以这里不好搞 
	 */
	//方法：解链后重新接链
	if( vUnit[iFound].iLeft != -1 )	//有兄长
	{
		vUnit[vUnit[iFound].iLeft].iRight = vUnit[iFound].iRight;
		if(vUnit[iFound].iRight != -1)//有弟
			vUnit[vUnit[iFound].iRight].iLeft = vUnit[iFound].iLeft;
		if(vUnit[iFound].iChild != -1)//有子
			DeleteChildTag(vUnit[iFound].iChild);
		//OK，释放节点（内存存储单元）
		FreeTag(iFound);
	}else if( vUnit[iFound].iParent != -1 ) //有父亲
	{
        	if(vUnit[iFound].iRight != -1)//有弟
        	{
			vUnit[vUnit[iFound].iRight].iLeft = -1;
			vUnit[vUnit[iFound].iRight].iParent = vUnit[iFound].iParent; 
			vUnit[vUnit[iFound].iParent].iChild = vUnit[iFound].iRight;//新子 
        	}else{
			vUnit[vUnit[iFound].iParent].iChild = -1;
		}
		if(vUnit[iFound].iChild != -1)//有子
			DeleteChildTag(vUnit[iFound].iChild);
        	FreeTag(iFound);
	}else{
		printf("failed to del a unit.\n");
		return false;
	}
	//Trim();
	return true;
}


/* 删除strDir元素拥有的所有子元素，及其本身 */
int CPrettyText::Delete(string strDir)
{
	int iOld = iCurrent;	//先保存当前元素，下面Enter后，会改变
	Enter( strDir );	//Enter
	int ret = Delete();
	iCurrent = iOld;	
	//返回原来元素 注意，这可能已经是不可用了
	if( vUnit[iCurrent].iLeft==-1 && vUnit[iCurrent].iParent== -1 )
		iCurrent = iHead;
	return ret;
}

/* 前一个元素 */
int CPrettyText::Previous()
{
	if( vUnit[iCurrent].iLeft != -1 )
	{
		iCurrent = vUnit[iCurrent].iLeft;
		return true;
	}
	return false;
}

/* 后一个元素 */
int CPrettyText::Next()
{
	if( vUnit[iCurrent].iRight != -1 )
	{
		iCurrent = vUnit[iCurrent].iRight;
		return true;
	}
	return false;
}

/* 父亲 */
int CPrettyText::Parent()
{
	while( vUnit[iCurrent].iLeft != -1 )
		iCurrent = vUnit[iCurrent].iLeft;

	if( vUnit[iCurrent].iParent != -1 )
	{
		iCurrent = vUnit[iCurrent].iParent;
		return true;
	}
	return false;
}

/* 孩子 */
int CPrettyText::Child()
{
	if( vUnit[iCurrent].iChild != -1 )
	{
		iCurrent = vUnit[iCurrent].iChild;
		return true;
	}
	return false;
}

/* EOF */
int CPrettyText::IsEnd()
{
	if( vUnit[iCurrent].iRight == -1 )
	{
		return true;
	}
	else
	{
		return false;
	}
}


// 当前记录中最后一个 
int CPrettyText::Last()
{
	/* TODO (#1#): Implement CPrettyText::Last() */
	while(vUnit[iCurrent].iRight!=-1)
	   iCurrent = vUnit[iCurrent].iRight;
	return iCurrent;
}


/* BOF */
int CPrettyText::IsBegin()
{
	if( vUnit[iCurrent].iLeft == -1 )
		return true;
	else 
		return false;
}


/* 递归删除所有子元素 */
int CPrettyText::DeleteChildTag(int iNum)
{
    //向右横扫所有节点
    if( vUnit[iNum].iRight != -1 )//有弟
        DeleteChildTag(vUnit[iNum].iRight);
    if( vUnit[iNum].iChild != -1 )//有子
        DeleteChildTag(vUnit[iNum].iChild);
    FreeTag(iNum);
	return true;
}

/* 回收节点，节约资源，保护环境，人人有责 */
int CPrettyText::FreeTag(int iNum)
{
	vUnit[iNum].iLeft = -1;
	vUnit[iNum].iParent = -1;
	/* 
	 * 2006-7-22 注释
	 * 这里暂时没什么好方法, 已经用Trim()代替
	 * 理由：失败
	 */
	//
	/*
	// 方法是把最后一个填充要删除的，再删除最后一个，然后调整链接 
	if( iNum == iCurrent )	//不能把当前删除了，如果真的删了，只能调回开头
	{
		iCurrent = iHead;
	}
	*/
	return true;
}

string& CPrettyText::Name()
{
	return vUnit[iCurrent].strName;
}

/* 显示所有元素信息，调试用 */
int CPrettyText::Dump()
{
	for( int i=0; i< vUnit.size(); i++ )
        printf("Num:%d  Name:%s  Value:%s  Left:%d  Right:%d  Parent:%d  Child:%d\n", i,
        vUnit[i].strName.c_str(), vUnit[i].strValue.c_str(), 
        vUnit[i].iLeft, vUnit[i].iRight, vUnit[i].iParent, vUnit[i].iChild );
	return 0;
}

/* 删除所有内存中废弃的元素 */
int CPrettyText::Trim()
{
	// iNum=0这是不行D，那是族长，怎可以随便删除呢
	for( int iNum=1; iNum< vUnit.size(); iNum++ )
		if( vUnit[iNum].iLeft == -1 && vUnit[iNum].iParent == -1 )
		{
			if(iNum==vUnit.size()-1)
			{
    			vUnit.pop_back();
    			continue;
			}
			vUnit[iNum] = vUnit[vUnit.size()-1];
			//cout<<"Move from "<<vUnit.size()-1<<" to "<<iNum<<endl;
			vUnit.pop_back();
			// 下面是很重要的
			if(vUnit[iNum].iParent != -1)
				vUnit[vUnit[iNum].iParent].iChild = iNum;
			if(vUnit[iNum].iChild != -1)
				vUnit[vUnit[iNum].iChild].iParent = iNum;
			if(vUnit[iNum].iLeft != -1)
				vUnit[vUnit[iNum].iLeft].iRight = iNum;
			if(vUnit[iNum].iRight != -1)
				vUnit[vUnit[iNum].iRight].iLeft = iNum;
		}	
	return true;
}



/* 查找指定表下名为strName的记录 */
int CPrettyText::Find(string strName)
{
	strName = PString::ToLower(strName);
	int i = vUnit[iCurrent].iChild;
	//遍历所有兄弟	
	while( i != -1 )
	{
		if( strName == vUnit[i].strName )
		{
			iCurrent = i;
			return true;
		}
		i = vUnit[i].iRight;	//下一个兄弟
	}
	return false;
}

/* 格式化危险字符 */
void CPrettyText::FormatText(string &strText, int M)
{
	switch(M)
	{
	case TEXT_IN:
		//PString::Replace(strText, "&nbsp;", " ");
		//PString::Replace(strText, "&rt;", "\n");
		//PString::Replace(strText, "\\&quot;", "\"");
		PString::Replace(strText, "&quot;", "\"");
		PString::Replace(strText, "&gt;", ">");
		PString::Replace(strText, "&lt;", "<");
		PString::Replace(strText, "&amp;", "&");
		break;
	case TEXT_OUT:
		PString::Replace(strText, "&", "&amp;");
		PString::Replace(strText, "<", "&lt;");
		PString::Replace(strText, ">", "&gt;");
		PString::Replace(strText, "\"", "&quot;");
		//PString::Replace(strText, "\n;", "&rt;");
		//PString::Replace(strText, " ", "&nbsp;");
		break;
	}
}


/**********************************************/
/* 下面开始是PrettyText提供的几个类型转换函数 */
/**********************************************/
namespace PString{
    void Replace(string & strBig, const string & strsrc, const string &strdst)
    {
        string::size_type pos=0;
        string::size_type srclen=strsrc.size();
        string::size_type dstlen=strdst.size();
        while( (pos=strBig.find(strsrc, pos)) != string::npos){
                strBig.replace(pos, srclen, strdst);
                pos += dstlen;
        }
    }
	string ToString(int iNum)
	{
		char buf[10];
		itoa(iNum, buf, 10);
		return buf;
	}
	string ToLower(const string& str)
	{
		string strRet;
		for( int i=0 ; i < str.length(); i++)
			if( str[i]>='A' && str[i]<='Z' )
				strRet += str[i]-'A' + 'a';
			else
				strRet += str[i];
		return strRet;
	}
	string ToUpper(const string& str)
	{
		string strRet;
		for( int i=0 ; i < str.length(); i++)
			if( str[i]>='a' && str[i]<='z' )
				strRet += str[i]-'a' + 'A';
			else
				strRet += str[i];
		return strRet;
	}

}
