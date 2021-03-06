// KeyboardDataList.cpp : implementation file
//

#include "stdafx.h"
#include "KeyboardDataList.h"

#include "WndKeyboard.h"
#include <process.h> 
#include "GeneralHelper.h"
#include "WinnerApplication.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
//const char Default_Item_Count = 9;

struct ListBoxItemData
{
	short m_nMask;
	long  m_lParam;
	ListBoxItemData()
	{
		memset(this,0,sizeof(ListBoxItemData));
	}

	ListBoxItemData(short nMask,long lParam)
	{
		m_nMask  = nMask;
		m_lParam = lParam;
	}
};
CArray<ListBoxItemData*,ListBoxItemData*> g_ayListBoxItemData;
void RemoveAllListItemData()
{
	for(int i = 0; i < g_ayListBoxItemData.GetSize(); i++ )
	{
		delete g_ayListBoxItemData.GetAt(i);
	}
	g_ayListBoxItemData.RemoveAll();
}

ListBoxItemData* GetListItemData(int nCurItem,short nMask,long lParam)
{
	ListBoxItemData* pData;
	if( nCurItem >= 0 && nCurItem < g_ayListBoxItemData.GetSize() )
	{
		pData = g_ayListBoxItemData.GetAt(nCurItem);
		pData->m_nMask  = nMask;
		pData->m_lParam = lParam;
	}
	else
	{
		pData = new ListBoxItemData(nMask,lParam);
		g_ayListBoxItemData.Add(pData);
	}
	return pData;
}

ListBoxItemData* GetListItemData(short nMask,long lParam)
{
	ListBoxItemData* pData;

  for(int i = 0; i < g_ayListBoxItemData.GetSize(); i++)
  {
		pData = g_ayListBoxItemData.GetAt(i);
    if(pData->m_lParam == lParam && pData->m_nMask == nMask)
      return pData;
  }

	pData = new ListBoxItemData(nMask,lParam);
	g_ayListBoxItemData.Add(pData);

	return pData;
}
/////////////////////////////////////////////////////////////////////////////
// CKeyboardDataList
HWND  g_hEditUpdateParentWnd = NULL;

HSCurKeyInfo  m_strData;
CString		  m_strKey;
WORD          g_nPageType=PageInfo_Quote;
UINT          m_ShowQuickTradeKey = 0;
BOOL          g_bCharReturn= FALSE;
#ifdef SUPPORT_2004_03_23_CHANGE
CString		  m_OldstrKey;
#endif

static char m_cHSUsrDefShortKeyMark[] = "$"; // 用户定义键标识
HSShowKeyboardInfo m_sHSShowKeyboardInfo;


CKeyboardDataList::CKeyboardDataList()
{
	m_pFont = NULL;
	
	/*m_pExpressMap = NULL;*/

	m_nStopThread = 0;
	m_lSinkID = 0;
	m_dStyle  = HSShowKeyboardInfo::all;
	m_ayOrder.RemoveAll();
	m_bWaitFind = FALSE;
}

CKeyboardDataList::~CKeyboardDataList()
{
	ExitDataThread();
	m_ayStockInfo.RemoveAll();
	RemoveAllListItemData();
	RemoveArray();
	RemoveMap();
	if( m_pFont )
	{
		delete m_pFont;
	}
	for ( int i=0; i<m_ayStockPos.GetCount(); i++)
	{
		HSStockPosition* pos = m_ayStockPos.GetAt(i);
		if (pos )
		{
			if ( pos->m_psiInfo)
			{
				pos->m_psiInfo->RemoveAll();
				delete pos->m_psiInfo;
			}
			delete pos;
		}
	}
	m_ayStockPos.RemoveAll();
	for ( int i=0;i<m_ayKey.GetCount();i++)
	{
		HSDefaultKeyInfo* pKey = m_ayKey.GetAt(i);
		if ( pKey && !IsBadReadPtr(pKey, 1))
		{
			if ( pKey->m_pInfo && !IsBadReadPtr(pKey->m_pInfo,1))
				delete pKey->m_pInfo;
			delete pKey;
		}
	}
	m_ayKey.RemoveAll();

	if ( m_pDataSource )
		m_pDataSource->HSDataSource_DataSourceUnInit(m_lSinkID);
}


BEGIN_MESSAGE_MAP(CKeyboardDataList, CKeyboardListBase)
	//{{AFX_MSG_MAP(CKeyboardDataList)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CHAR()
	ON_MESSAGE(MsgInitStocks, OnInitStocks)
	//}}AFX_MSG_MAP
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CKeyboardDataList message handlers

int CKeyboardDataList::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CKeyboardListBase::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_pFont = CGeneralHelper::CreateFont("宋体",14,0); // -90
	this->SetFont(m_pFont);
	return 0;
}

void CKeyboardDataList::OnDestroy() 
{
	CKeyboardListBase::OnDestroy();
	
	// TODO: Add your message handler code here
	
}

void CKeyboardDataList::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	CWnd* pWnd = GetParent();
	if( pWnd != NULL )
	{
		if( nChar == VK_RETURN )
		{
			if( !((CWndKeyboard*)pWnd)->IsStyle(DS_ALWAYSSHOW) )
			{
				((CWndKeyboard*)pWnd)->ShowWindow(SW_HIDE);
			}
			if( GetCount() <= 0 /*&& m_ayStockInfo.GetCount() <= 0*/)
				return;

		
			int nIndex = this->GetCurSel( );
			if( nIndex == -1 )
				nIndex = 0;
		
			LVITEM item;
			char   text[255];
			memset(&item,0,sizeof(LVITEM));
				
			item.lParam = (LPARAM)GetItemDataPtr(nIndex);
			ListBoxItemData* ppp = (ListBoxItemData*)item.lParam;
			if( ppp == NULL )
				return;

			item.iImage = ppp->m_nMask;
			item.lParam = ppp->m_lParam;

			GetText(nIndex,text);

			long wParam;
			LPARAM lParam = (LPARAM)&m_strData;

			m_strData.Empty();
	
			if( item.iImage == KEYINFO_STOCK ) // 股票
			{		
				lParam = item.lParam;
				if ( GetAsyncKeyState(VK_CONTROL) < 0 )
				{
					wParam = m_strData.m_nOpen = HSCurKeyInfo::KeyStockAdd;
				}
				else				
					wParam = m_strData.m_nOpen = HSCurKeyInfo::KeyStock;
			}
			else if( item.iImage == KEYINFO_EXPRESS ) // 公式
			{
				lParam = item.lParam;
				wParam = m_strData.m_nOpen = HSCurKeyInfo::UpdateExpressData;
			}
			else if( item.iImage == KEYINFO_USERINFO ) // 外部定义
			{
				HSDefaultKeyInfo* pKey = (HSDefaultKeyInfo*)item.lParam;
				m_strData.Copy(pKey->m_pInfo);
				wParam = m_strData.m_nOpen = HSCurKeyInfo::KeyUsedViewInfo;
			}
			else if(item.iImage == KEYINFO_COMMAND) // 特殊页面
			{
				wParam = HSCurKeyInfo::KeyCommand;
				HSDefaultKeyInfo* pKey = (HSDefaultKeyInfo*)item.lParam;
				if(g_hEditUpdateParentWnd != NULL)
				{
					::SendMessage(g_hEditUpdateParentWnd,WM_KEYBOARD_NOTIFY, wParam, pKey->m_CmdID);
				}
				return;
			}
			else // 其他：快键
			{
				m_strData.m_nOpen = HSCurKeyInfo::KeyDataQuick;
				wParam = HSCurKeyInfo::KeyDataQuick;

				CString strText;
				strText.Format("%s",text);
				strText.Trim();
				nIndex = strText.Find(" ");
				if( nIndex != -1 )
				{
					// 如果是用户外部定义的键盘，则传送名称。
					int nNextIndex = strText.Find(m_cHSUsrDefShortKeyMark);
					if( nNextIndex != -1 )
					{
						strText = strText.Mid(nNextIndex + 1);
						wParam = m_strData.m_nOpen = HSCurKeyInfo::OpenUsrDefPage;
					}
					else
					{
						strText = strText.Left(nIndex);
					}
				}
				m_strData.Copy(strText);
			}

			if(g_hEditUpdateParentWnd != NULL)
			{
 				::SendMessage(g_hEditUpdateParentWnd,WM_KEYBOARD_NOTIFY/*HSKEYBOARDMSG*/,
 					wParam,lParam);
			}
			else
			{
				return;
			}

			//
			if( HSCurKeyInfo::UpdateExpressData == m_strData.m_nOpen )
			{
				if( !((CWndKeyboard*)pWnd)->IsStyle(DS_ALWAYSSHOW) )
				{
					((CWndKeyboard*)pWnd)->ShowWindow(SW_HIDE);
				}
				return;
			}
		}
		else if(nChar == VK_LEFT || nChar == VK_RIGHT)
		{
			if(pWnd->SendMessage(hxUSER_EDITMSG,nChar,(LPARAM)this) )
				return;
		}
	}

	CKeyboardListBase::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CKeyboardDataList::InitKeyboard()
{	
	m_pKeyboardCfg = (IHsKeyboardCfg*)CWinnerApplication::GetObject(OBJ_KEYBOADR);
	m_pFormulaMan = (IFormulaMan*)CWinnerApplication::GetObject(OBJ_FORMULAMAN);
	m_pDataSource = (IDataSourceEx*)CWinnerApplication::GetObject(OBJ_HSDATAENGINE);

	if ( m_pDataSource)
		m_lSinkID = m_pDataSource->HSDataSource_DataSouceInit(this, NULL);
	int lRetLen = sizeof(AskData);
	AskData* pskData = (AskData*)new char[lRetLen];
	memset(pskData, 0, lRetLen);				
	pskData->m_nType = RT_SERVERINFO;
	m_pDataSource->HSDataSource_RequestAsyncData(m_lSinkID, (char*)pskData,lRetLen,10000);
	delete[] pskData;

	CString strName;
	CString strValue;
	CStringArray strCutShort;
	CString strSection = "金阳光";

	BOOL bUserDef = FALSE;

 	if ( m_pKeyboardCfg )
 	{
 		m_pKeyboardCfg->GetOrderMarket(m_ayOrder,strSection);
 		m_pKeyboardCfg->GetCutShort(strCutShort);
 	}
 	for ( int i=0; i<strCutShort.GetCount(); i++)
 	{
 		CString strKeyID = strCutShort.GetAt(i++);
 		CString strKeyName = strCutShort.GetAt(i++);
 		int     CmdID = atoi(strCutShort.GetAt(i));
 		if (CmdID)
 			AddUsrShortKeyEx(strKeyID,strKeyName,CmdID);
 		else
 			AddUsrShortKey(strKeyID,strKeyName);
 	}
 	//获取股票代码
 	CArray<CodeInfo> ayCodeinfo;
 
 	for (int i=0; i<m_ayOrder.GetCount(); i++)
 	{
 		int mardetType;
 		char *str;
 		CodeInfo info;
 		CString strOrder = m_ayOrder.GetAt(i);
 		if ( !strOrder.IsEmpty() )
 		{
 			HSStockPosition* pPos = new HSStockPosition;
 			pPos->m_psiInfo = new CArray<StockUserInfo>;
 			CString strText;
 			strText.Format("0x%s",strOrder);
 			mardetType =  (int)strtol(strText.GetBuffer(), &str, 16);
 			pPos->m_lBegin = 0;
 			pPos->m_market = mardetType;
 			int count = 0;
 			if ( m_pDataSource )
 				count = m_pDataSource->HSDataSourceEx_GetMarketStocksInfo(mardetType, *(pPos->m_psiInfo));
 			pPos->m_lEnd = count;
 			m_ayStockPos.Add(pPos);
 		}
 		
 	}
	if (m_pFormulaMan)
	{	
		m_pFormulaMan->GetExpProp(m_ayExpProp);
	}
}

int CKeyboardDataList::FindUsrShortKey(CString strKey,CString strNotes)
{
	HSDefaultKeyInfo* pKey;
	for(int i = 0; i < m_ayKey.GetSize(); i++)
	{
		pKey = m_ayKey.GetAt(i);
		if( !strKey.CompareNoCase(pKey->m_strKey) &&
			!strNotes.CompareNoCase(pKey->m_strNotes) )
		{
			return i;
		}
	}

	return -1;
}

int CKeyboardDataList::FindShortCut(CString strKey)
{
  HSShortCutInfo *pShortcut;

  for(int i = 0; i < m_ayShortCut.GetSize(); i++)
  {
    pShortcut = m_ayShortCut.GetAt(i);
    if(!pShortcut->m_strKey.CompareNoCase(strKey))
      return i;
  }

  return -1;
}

void CKeyboardDataList::AddShortCut(CString strKey, CString strMap)
{
  HSShortCutInfo *pShortcut;
  int Index = FindShortCut(strKey);

  if(Index != -1)
  {
    pShortcut = m_ayShortCut.GetAt(Index);
    pShortcut->m_strMap = strMap;
    return;
  }

  pShortcut = new HSShortCutInfo;
  pShortcut->m_strKey = strKey;
  pShortcut->m_strMap = strMap;

  m_ayShortCut.Add(pShortcut);

  return;
}

int CKeyboardDataList::AddUsrShortKey(CString strKey,CString strNotes,BOOL bDel /*= FALSE*/)
{

	strKey.Trim();
	strKey.MakeUpper();

	strNotes.Trim();
	strNotes.MakeUpper();

	int nIndex = FindUsrShortKey(strKey,strNotes);

	HSDefaultKeyInfo* rValue;
	if( bDel )
	{
		if( nIndex != -1 )
		{
			rValue = m_ayKey.GetAt(nIndex);
			m_ayKey.RemoveAt(nIndex);
			delete rValue;
		}
	}
	else if( nIndex == -1 )
	{
		rValue = new HSDefaultKeyInfo(strKey,strNotes);
		m_ayKey.Add(rValue);
	}

	return 1;
}

int CKeyboardDataList::AddUsrShortKeyEx(CString strKey,CString strNotes,UINT CmdID,BOOL bDel)
{

	strKey.Trim();
	strKey.MakeUpper();

	strNotes.Trim();
	strNotes.MakeUpper();

	int nIndex = FindUsrShortKey(strKey,strNotes);

	HSDefaultKeyInfo* rValue;
	if( bDel )
	{
		if( nIndex != -1 )
		{
			rValue = m_ayKey.GetAt(nIndex);
			m_ayKey.RemoveAt(nIndex);
			delete rValue;
		}
	}
	else if( nIndex == -1 )
	{
		rValue = new HSDefaultKeyInfo(strKey,strNotes);
		m_ayKey.Add(rValue);
	}

  rValue->m_CmdID = CmdID;

	return 1;
}

DWORD WINAPI Filter(LPVOID pParameter)
{	
	CKeyboardDataList* pList = (CKeyboardDataList*)pParameter;
	if(pList == NULL) 
	{
		pList->m_nStopThread = 0;
		return 0;
	}

	try
	{
		pList->FilterData();
	}
	catch(...) {}

	pList->m_nStopThread = 0;

	return 0;
}
void CKeyboardDataList::FilterData()
{	
	try
	{
		m_nStopThread = -2;
		int  nKeyLen;
	/*	int  i*/;
		int  bFind;
		int  nFindIndex;
		BOOL bFindExpress = TRUE;
		BOOL bFindCutKey = TRUE;
		while( m_nStopThread != 2 )
		{
			if( m_bWaitFind )
			{
				//如果是按下回车键，等待计算完成再响应回车事件
				if (g_bCharReturn)
				{
					g_bCharReturn = FALSE;
					m_strKey = "";
					m_ayStockInfo.RemoveAll();
					SendMessage(WM_KEYDOWN,VK_RETURN,0);
				}
				Sleep(50);
				continue;
			}

__BeginFind_:

			if( m_strKey.IsEmpty() )
			{
				m_OldstrKey.Empty();
				ResetContent();
				m_bWaitFind = TRUE;
				continue;
			}	
			
			BOOL bHaveItem = FALSE;
			BOOL bKeyBack  = FALSE;
			BOOL bFirstPress = FALSE;
	
		
			//如果不是按下退格键，则查找时在上次查找的结果中找，否则在第一次中找
			if ( m_strKey.Find(m_OldstrKey) < 0 || (m_strKey.GetLength() <= 1 && !m_OldstrKey.IsEmpty()))
			{
				bKeyBack = TRUE;
				m_ayStockInfo.RemoveAll();
				m_ayCutShort.RemoveAll();
			}
			//第一次按下字符时
			if (m_strKey.GetLength() == 1 && m_OldstrKey.GetLength() == 0 )
			{
				bFindCutKey = TRUE;
				bFindExpress = TRUE;
				bFirstPress = TRUE;
				ResetContent();
 				m_ayStockInfo.RemoveAll();
 				m_ayCutShort.RemoveAll();
			}
		
			CString strText,strKey;
			strKey.Format("%s",m_strKey);
			strKey.MakeUpper();
			int  nCurKeyLen = strKey.GetLength();

			int  nIndex = 0;
			int  nUnique = -1;

			int  nStart = -1;
			int  nEnd   = -1;
			BOOL bFirst = 0;
			static int nPos;
			// 在常用键盘中查找
			if( IsStyle(HSShowKeyboardInfo::key) || 
				IsStyle(HSShowKeyboardInfo::all) )
			{	
				if ( bFindCutKey || bKeyBack)
				{
					m_ayCutShort.RemoveAll();
					HSDefaultKeyInfo* pKey;				
					for( nPos = 0; nPos < m_ayKey.GetSize(); nPos++)
					{
						if( m_bWaitFind || nCurKeyLen != strKey.GetLength() )
							goto __BeginFind_;
						pKey = m_ayKey.GetAt(nPos);

						if( pKey )
						{
							nFindIndex = pKey->IsFind(strKey);
							if( nFindIndex == 0 ) 
							{
								CutShortInfo cutshort;
								cutshort.m_pKeyInfo = pKey;
								
								bHaveItem = TRUE;
								strText.Format("%6s %s",pKey->m_strKey,pKey->m_strNotes);
								CString strNote = pKey->m_strNotes;
								int nBuy = strNote.CompareNoCase("闪电买入");
								int nSell = strNote.CompareNoCase("闪电卖出");

								if( pKey->m_pInfo != NULL )
								{
									cutshort.m_nPageID = KEYINFO_USERINFO;
									m_ayCutShort.Add(cutshort); //KEYINFO_USERINFO
								}
								else if(pKey->m_CmdID)
								{
									if (  g_nPageType != PageInfo_Tech)
									{
										if ( strText.Find("叠加大盘指数")!=-1 )  //不是个股分时图不能叠加大盘
												continue;
										if ( pKey->m_CmdID >= KeyBegin+67 && pKey->m_CmdID <= KeyBegin+68)
											continue;
										//报价界面 显示闪电下单快捷键
										if ((nBuy ==0 || nSell ==0))
										{
											cutshort.m_nPageID = KEYINFO_COMMAND;
											m_ayCutShort.Add(cutshort); //KEYINFO_COMMAND
										}
										else if (pKey->m_CmdID >= KeyOneMinute && pKey->m_CmdID <= KeyMultiDayData )
										{
										}
										else
										{
											cutshort.m_nPageID = KEYINFO_COMMAND;
											m_ayCutShort.Add(cutshort); //KEYINFO_COMMAND
										}
									}
									else
									{
										//if ( strText.Find("叠加大盘指数")!=-1 )  //不是个股分时图不能叠加大盘
										//	continue;
										cutshort.m_nPageID = KEYINFO_COMMAND;
										m_ayCutShort.Add(cutshort); //KEYINFO_COMMAND
									}
								}
								else
								{
									cutshort.m_nPageID = KEYINFO_QUICKDATA;
									m_ayCutShort.Add( cutshort); //KEYINFO_QUICKDATA
								}

								if( nUnique == -1 )
								{
									nKeyLen = strKey.GetLength();
									strText.TrimLeft();
									strText = strText.Left(nKeyLen);
									if( !strText.CompareNoCase(strKey) )
										nUnique = nIndex;
								}
								nIndex++;
							}
						}
					}
				}
				
				if (bHaveItem)
					bFindCutKey = TRUE;
				else 
					bFindCutKey = FALSE;	
			}

			bFind = FALSE;
			int nExpStart = nIndex;
		//	 在公式中查找
 			if( IsStyle(HSShowKeyboardInfo::express) || 
  				IsStyle(HSShowKeyboardInfo::all) )
  			{
				m_ayNameProp.RemoveAll();
				if ( bFindExpress || bKeyBack)
				{
					for (int i=0; i<m_ayExpProp.GetCount(); i++)
					{
						if( m_bWaitFind || nCurKeyLen != m_strKey.GetLength() )
							goto __BeginFind_;		
						ExpPropery* pExpress = m_ayExpProp.GetAt(i);
						if (pExpress)
						{
							CString strName = pExpress->m_strName;
							strName.MakeUpper();
							int nFindIndex = strName.Find(m_strKey);
							if ( nFindIndex == 0)
								m_ayNameProp.InsertAt(0,pExpress);
							else if ( nFindIndex != -1)
							{
								m_ayNameProp.Add(pExpress);
							}
						}
					}
				}		
		
				if ( m_ayNameProp.GetCount()>0 )
				{
					bHaveItem = TRUE;
					bFindExpress = TRUE;
				}
				else
					bFindExpress = FALSE;			
  			}
 			

			int nCurIndex = nIndex;
			bFind = FALSE;
			CString szMarch;
			CArray<StockUserInfo*,StockUserInfo*> ayAdd;
			CArray<StockUserInfo*,StockUserInfo*> ayLastAdd;
			// 在股票中查找
			
			BOOL bTest = FALSE;
			if( IsStyle(HSShowKeyboardInfo::stock) || 
				IsStyle(HSShowKeyboardInfo::all) )
			{	
				//不是按下退格键，在上次查找到的结果中找
				if ( m_ayStockInfo.GetCount() > 0 )
				{	
					for (int i=0; i<m_ayStockInfo.GetCount(); i++)
					{
						int nBetter = -1; 
						StockUserInfo* pStockInfo = m_ayStockInfo.GetAt(i);
						if (pStockInfo && !IsBadReadPtr(pStockInfo,1))
						{
							try
							{
								if( m_bWaitFind || nCurKeyLen != m_strKey.GetLength() )
									goto __BeginFind_;	
								if ( m_pDataSource )
									nFindIndex = m_pDataSource->HSDataSource_FindStockFromKey(pStockInfo, m_strKey,bFind,nStart,nEnd, szMarch,nBetter,nCurIndex);
							//	nFindIndex = pStockInfo->FindFromTail(m_strKey,bFind,nStart,nEnd, szMarch,nBetter,nCurIndex);
							}					
							catch (...)
							{
								nCurKeyLen = nCurKeyLen;
								nFindIndex = -1;
							}
							if( nFindIndex == 0 )
							{	
								bHaveItem = TRUE;	
								if(nCurKeyLen == 4 )
								{
									if (  pStockInfo->IsSz4(strKey))
									{
										int nInsert = FindStockIndex(MakeMainMarket(SZ_BOURSE));
										ayAdd.InsertAt(nInsert,pStockInfo);
									}
									else
										ayAdd.Add(pStockInfo);
									
								}
								else
								{
									if (nBetter >= 0)
										ayAdd.InsertAt(nBetter,pStockInfo);
									else
										ayAdd.Add(pStockInfo);
								}    					
							}
							else if( nFindIndex != -1 )
							{
								ayLastAdd.Add(pStockInfo);
							}
							nIndex++;
							nBetter = 0;
						}
					}	
					m_ayStockInfo.RemoveAll();
 					if( ayAdd.GetSize() >= 0 )
 					{	 					
  						m_ayStockInfo.Copy(ayAdd);	
 					}
					for (int j=0; j<ayLastAdd.GetCount(); j++)
					{
						m_ayStockInfo.Add(ayLastAdd.GetAt(j));
					}					
				}
				else
				{
					for(int nPos = 0; nPos < m_ayStockPos.GetSize(); nPos++)
					{
						HSStockPosition* pCurPos = m_ayStockPos.GetAt(nPos);
						if ( pCurPos && !IsBadReadPtr(pCurPos,1))
						{

							if( !(WhoMarket(pCurPos->m_market,STOCK_MARKET) ||
								WhoMarket(pCurPos->m_market,FUTURES_MARKET) ||
								WhoMarket(pCurPos->m_market,FOREIGN_MARKET) ||
								WhoMarket(pCurPos->m_market,WP_MARKET) ||
								WhoMarket(pCurPos->m_market,HK_MARKET)) )
							{
								continue;
							}	

							for(int i = 0; pCurPos->m_psiInfo && 
								i < pCurPos->m_psiInfo->GetSize() ; i++)
							{

								StockUserInfo* pStockInfo = &(pCurPos->m_psiInfo->GetAt(i));
								int nBetter = -1; 
								if (pStockInfo && !IsBadReadPtr(pStockInfo,1))
								{
									try
									{
										if( m_bWaitFind || nCurKeyLen != m_strKey.GetLength() )
											goto __BeginFind_;	
										if ( m_pDataSource )
											nFindIndex = m_pDataSource->HSDataSource_FindStockFromKey(pStockInfo, m_strKey,bFind,nStart,nEnd, szMarch,nBetter,nCurIndex);
										/*nFindIndex = pStockInfo->FindFromTail(m_strKey,bFind,nStart,nEnd, szMarch,nBetter,nCurIndex);*/
									}					
									catch (...)
									{
										nCurKeyLen = nCurKeyLen;
										nFindIndex = -1;
									}
									if( nFindIndex == 0 )
									{	
										bHaveItem = TRUE;	
										if(nCurKeyLen == 4)
										{
											if (  pStockInfo->IsSz4(strKey))
											{
												int nInsert = FindStockIndex(MakeMainMarket(SZ_BOURSE));
												ayAdd.InsertAt(nInsert,pStockInfo);
											}
											else
												ayAdd.Add(pStockInfo);
										}
										else
										{
											//int nInsert = FindStockIndex(MakeMainMarket(pStockInfo->m_ciStockCode.m_cCodeType));
											if (nBetter >= 0)
												ayAdd.InsertAt(nBetter,pStockInfo);
											else
												ayAdd.Add(pStockInfo);
										}    					
									
									}
									else if( nFindIndex != -1 )
									{
										ayLastAdd.Add(pStockInfo);
									}
									nIndex++;
									nBetter = 0;
								}
							}	
						}
					}
				
					if( ayAdd.GetSize() >= 0 )
					{						
						m_ayStockInfo.Copy(ayAdd);						
					}
					for (int j=0; j<ayLastAdd.GetCount(); j++)
					{
						m_ayStockInfo.Add(ayLastAdd.GetAt(j));
					}
				}
					
				// 匹配找到的
  				if( ayAdd.GetSize() > 0 || m_ayStockInfo.GetSize() > 0)
  				{
  					bHaveItem = TRUE;
  				}
			}

		
			//插入item
			CWnd* pWnd = GetParent();
			if( pWnd != NULL )
				pWnd->ShowWindow(SW_SHOW);
			ResetContent();
			//快键
			for (int i=0;i<m_ayCutShort.GetCount(); i++)
			{
				CutShortInfo* pCutInfo = &m_ayCutShort.GetAt(i);
				if (pCutInfo)
				{
					HSDefaultKeyInfo* pKey = pCutInfo->m_pKeyInfo;
					if ( pKey )
					{
						CString strText;
						strText.Format("%6s %s",pKey->m_strKey,pKey->m_strNotes);
						AddItem(nIndex,strText,pCutInfo->m_nPageID,(LPARAM)pKey,FALSE,0);	
					}
				}
			}
			//公式
			nExpStart = m_ayCutShort.GetCount();
			if ( m_ayNameProp.GetCount() > 0)
				AddExpress(nExpStart, m_ayNameProp);
			//股票
			for( nPos = 0; nPos < m_ayStockInfo.GetSize(); nPos++ )
			{
				if( m_bWaitFind || nCurKeyLen != m_strKey.GetLength() )
					goto __BeginFind_;

				StockUserInfo* pCurStock  = m_ayStockInfo.GetAt(nPos);
				if (pCurStock && !IsBadReadPtr(pCurStock,1))
				{
					CString str="";
					if ( m_pDataSource)
						str = m_pDataSource->HSDataSource_GetStockPyjc(pCurStock);
					strText.Format( "%6s %s %s",pCurStock->m_ciStockCode.GetCode(),
						pCurStock->GetName(), /*pCurStock->GetDefaultPyjc()*/str );
					if(nCurKeyLen == 4)
						AddStock(nIndex,strText,KEYINFO_STOCK,(DWORD)pCurStock,pCurStock->IsSz4(strKey),nFindIndex);
					else
						AddItem(nIndex,strText,KEYINFO_STOCK,(DWORD)pCurStock,FALSE,nFindIndex);
				
					nIndex++;
				}					
			}
	
			if( bHaveItem == FALSE  )
			{
			}
			else
			{
				m_OldstrKey.Format("%s", m_strKey);
			}

			if( !strKey.CompareNoCase(m_strKey) )
			{
				m_bWaitFind = TRUE;
			}

		}
	}
	catch(...)
	{
		m_nStopThread = -1;
		return;
	}

	m_nStopThread = -1;

	return;
}

void CKeyboardDataList::RemoveItem(int nItem)
{

#ifdef _Use_ListCtrl_
	int nCount = this->GetItemCount();
	while( nItem < nCount /*&& nCount > Default_Item_Count*/ )
	{
		DeleteItem(nCount-1);
		nCount = this->GetItemCount();
	}
#else

	//ResetContent();

#endif
}
void  CKeyboardDataList::RemoveArray()
{
	m_ayNameProp.RemoveAll();
	m_ayCutShort.RemoveAll();
	m_ayStockInfo.RemoveAll();
}
void CKeyboardDataList::RemoveMap()
{
	for ( int i=0; i<m_ayStockPos.GetCount(); i++)
	{
		HSStockPosition* pPos = m_ayStockPos.GetAt(i);
		if ( pPos )
		{
			pPos->m_psiInfo->RemoveAll();
			delete pPos->m_psiInfo;
		}
		delete pPos;
	}
	m_ayStockPos.RemoveAll();
	for (int i=0; i<m_ayExpProp.GetCount();  i++)
	{
		ExpPropery* pExp = m_ayExpProp.GetAt(i);
		if (pExp)
			delete pExp;
	}
	m_ayExpProp.RemoveAll();
}

void CKeyboardDataList::AddItem(int nItem, LPCTSTR lpszItem, int nImage,
								LPARAM lParam,BOOL bFirst,int nAddPos /*= -1*/ )
{
	int nCurItem;

	if(bFirst)
	{
		if (nAddPos >= 1)
			nCurItem = InsertString(nAddPos, lpszItem);
		else
			nCurItem = InsertString(0, lpszItem);
	}
	else
		nCurItem = AddString(lpszItem);

	SetItemDataPtr(nCurItem,(void*)GetListItemData(nImage,lParam));
	if( nCurItem == 0 )
	{
		SetCurSel(nCurItem);
	}
}

int CKeyboardDataList::FindStockIndex(int type)
{
  StockUserInfo *pStock;
  ListBoxItemData *pData;
  int i;
  for(i = 0; i < this->GetCount(); i++)
  {
    pData = (ListBoxItemData *)GetItemDataPtr(i);
    if(pData->m_nMask != KEYINFO_STOCK)
      continue;

    pStock = (StockUserInfo *)pData->m_lParam;
    if(pStock->m_ciStockCode.m_cCodeType & type)
      continue;
    break;
  }

  return i;
}

void CKeyboardDataList::AddStock(int nItem, LPCTSTR lpszItem, int nImage,LPARAM lParam,
				                         BOOL bFirst,int nAddPos)
{
  int nCurItem;
  StockUserInfo *pStock = (StockUserInfo *)lParam;

  if(bFirst)
  {
    nCurItem = InsertString(FindStockIndex(SZ_BOURSE), lpszItem);
  }
  else
  {
    nCurItem = AddString(lpszItem);
  }

	SetItemDataPtr(nCurItem,(void*)GetListItemData(nImage,lParam));
	if( nCurItem == 0 )
	{
		SetCurSel(nCurItem);
	}
}
void  CKeyboardDataList::AddExpress(int nItem, const CArray<ExpPropery*,ExpPropery*>& ayNameprop)
{
	for ( int i=0;i<ayNameprop.GetCount(); i++)
	{	
		int nCount = GetCount();
		ExpPropery* pNameProp = ayNameprop.GetAt(i);
		int nCurItem;
		if ( pNameProp && !IsBadReadPtr(pNameProp,1))
		{
			CString strText;
			strText.Format("%6s %s",pNameProp->m_strName,pNameProp->m_strDescribe);

			CString sName = pNameProp->m_strName;
			sName.MakeUpper();
			if(!sName.CompareNoCase("MMLD") || !sName.CompareNoCase("LB"))
			{
					continue;
			}
			char type = pNameProp->m_dExpType;
			switch(type)
			{
			case Tech:
			 {
				/* nCurItem = InsertString(nItem >= nCount ? -1 : nItem, strText);*/
				// break;
			 }
			case Condition:
			case Exchange:
			default:
			 {
				 nCurItem = AddString(strText);
				 break;
			 }
			}
		}
		SetItemDataPtr(nCurItem,(void*)GetListItemData(KEYINFO_EXPRESS,(LPARAM)pNameProp));
		if( nCurItem == 0 )
		{
			SetCurSel(nCurItem);
		}
		
	}
}
void CKeyboardDataList::AddExpress(int nItem, LPCTSTR lpszItem, int nImage,
								                   LPARAM lParam,BOOL& bFirst,int nAddPos /*= -1*/ )
{
//   int nCurItem;
//   int nCount = GetCount();
// //	int nCurItem = AddString(lpszItem);
//   CExpression* pExpress = (CExpression *)lParam;
//   char type = pExpress->GetExpressType();
// 
//   switch(type)
//   {
//     case CExpression::Tech:
//     {
//       nCurItem = InsertString(nItem >= nCount ? -1 : nItem, lpszItem);
//       break;
//     }
//     case CExpression::Condition:
//     case CExpression::Exchange:
//     default:
//     {
//       nCurItem = AddString(lpszItem);
//       break;
//     }
//   }
// 
// 	SetItemDataPtr(nCurItem,(void*)GetListItemData(nImage,lParam));
// 	if( nCurItem == 0 )
// 	{
// 		SetCurSel(nCurItem);
// 	}
}

BOOL CKeyboardDataList::ExitDataThread()
{
	if( m_nStopThread != 0 && m_nStopThread != -1 )
	{
		m_nStopThread = 2;
		while (m_nStopThread > 0)
		{
			Sleep(20);
		}
	}
	m_nStopThread = 0;
	return TRUE;
}

BOOL CKeyboardDataList::CreateDataThread(CString strKey)
{
	if( m_sHSShowKeyboardInfo.m_cDispAll )
	{
		strKey = " ";
	}
	else if( !m_strKey.CompareNoCase(strKey) )
		return 0;

	if(strKey.IsEmpty() && g_bCharReturn) //回车事件
	{	
	//	if (m_bWaitFind)
			return 0;
	}
//	m_bWaitFind = TRUE;

	m_strKey    = strKey;
 
	m_bWaitFind = FALSE;

	if( m_nStopThread == 0 )
	{
		DWORD dwThreadID;
		m_nStopThread  = 1;
		HANDLE hThread = CreateThread(NULL, 0, Filter,(LPVOID)this, 0, &dwThreadID);
		if (hThread)
		{
			CloseHandle(hThread);
			
			while (m_nStopThread == 1)
			{
				Sleep(20);
			}
			if (m_nStopThread == -1)
			{
			}
		}
		else
		{
		}
	}

	return TRUE;

}

void CKeyboardDataList::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	
	SendMessage(WM_KEYDOWN,VK_RETURN,0);
	CWnd* pWnd = GetParent();
	if( pWnd != NULL )
	{
		pWnd->SendMessage(hxUSER_EDITMSG,VK_RETURN,(LPARAM)this);
	}

	CKeyboardListBase::OnLButtonDblClk(nFlags, point);
}

void CKeyboardDataList::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	CWnd* pWnd = GetParent();
	if( pWnd != NULL )
	{
		if(pWnd->SendMessage(hxUSER_EDITMSG,nChar,(LPARAM)this) )
		{
			return;
		}
	}

	CKeyboardListBase::OnChar(nChar, nRepCnt, nFlags);
}
//

CSize CKeyboardDataList::GetSize()
{	
	CDC* pDC = GetDC();
	TEXTMETRIC tm;
	pDC->GetTextMetrics(&tm);
	CSize size(tm.tmAveCharWidth*(6+8+4+1)+::GetSystemMetrics(SM_CYVSCROLL)+
		::GetSystemMetrics(SM_CYEDGE)*4,
		tm.tmHeight * 10); //20090417 YJT 修改宽度算法
	if (pDC) 
		ReleaseDC(pDC);
	//
	return size;
}

BOOL CKeyboardDataList::Create( DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID )
{
	BOOL bRet = CKeyboardListBase::Create( dwStyle, rect, pParentWnd, nID );

	ModifyStyleEx(0,WS_EX_STATICEDGE,0); //20090417 YJT 将 WS_EX_CLIENTEDGE 改为 WS_EX_STATICEDGE
	
	//SetImageList (&m_contentImages, LVSIL_SMALL);

	return bRet;
}

CListBoxKeyboard::CListBoxKeyboard()
{
	 m_bFirstEraseBk = TRUE;
}

CListBoxKeyboard::~CListBoxKeyboard()
{

}

int CListBoxKeyboard::CompareItem(LPCOMPAREITEMSTRUCT /*lpCompareItemStruct*/)
{

	// TODO:  Add your code to determine the sorting order of the specified items
	// return -1 = item 1 sorts before item 2
	// return 0 = item 1 and item 2 sort the same
	// return 1 = item 1 sorts after item 2

	/*ListData* pData1 = (ListData*)lpCompareItemStruct->itemData1;
	ListData* pData2 = (ListData*)lpCompareItemStruct->itemData2;
	long lRet = (long)(pData1->m_lData - pData2->m_lData);
	return lRet > 0 ? 1 : lRet == 0 ? 0 : -1;*/

	return 0;
}

void CListBoxKeyboard::DeleteItem(LPDELETEITEMSTRUCT lpDeleteItemStruct)
{
	// TODO: Add your specialized code here and/or call the base class

	CListBox::DeleteItem(lpDeleteItemStruct);
}

void CListBoxKeyboard::DrawItem(LPDRAWITEMSTRUCT lpDrawItemStruct)
{
	if( this->GetCount() <= 0 )
		return;

	/*if( lpDrawItemStruct->itemData == -1 || 
		lpDrawItemStruct->itemData == 0xffffffff ||
		lpDrawItemStruct->itemData == 0 )
	{
		return;
	}*/

	CDC* pDC = CDC::FromHandle(lpDrawItemStruct->hDC);
	int nOldMode = pDC->SetBkMode(TRANSPARENT);
	CRect rect = lpDrawItemStruct->rcItem;

	ListBoxItemData* pItem = (ListBoxItemData*)lpDrawItemStruct->itemData;
	CString strText;
	GetText(lpDrawItemStruct->itemID,strText);
	
	int nLineLength = 1;
	if(strText.GetLength() > 0)
	{
		rect.left += nLineLength;//rect.Height();
		CFont* pFont = GetFont();
		CFont* pOldFont = NULL;
		if(pFont != NULL)
		{
			pOldFont = pDC->SelectObject(pFont);
		}

		if ((lpDrawItemStruct->itemState & ODS_SELECTED) &&
			(lpDrawItemStruct->itemAction & (ODA_SELECT | ODA_DRAWENTIRE)))
		{
			pDC->FillSolidRect(rect, ::GetSysColor(COLOR_HIGHLIGHT));
			//pDC->SetTextColor( ::GetSysColor(COLOR_HIGHLIGHTTEXT) );
		}
  		else if (!(lpDrawItemStruct->itemState & ODS_SELECTED) &&
  			(lpDrawItemStruct->itemAction & ODA_SELECT))
		{
			pDC->FillSolidRect(rect, RGB(255,255,255));
		}
		
		if ( lpDrawItemStruct->itemState & ODS_SELECTED )
		{
			pDC->SetTextColor( ::GetSysColor(COLOR_HIGHLIGHTTEXT));
		}
		else if( pItem && pItem->m_nMask == KEYINFO_STOCK)
		{
			pDC->SetTextColor( RGB(0, 0, 255) );
		}
		else if( pItem && pItem->m_nMask == KEYINFO_EXPRESS )
		{
			pDC->SetTextColor( RGB(64,128, 128) );
		}

		else if (pItem && (pItem->m_nMask == KEYINFO_USERINFO || pItem->m_nMask == KEYINFO_COMMAND
				|| pItem->m_nMask == KEYINFO_QUICKDATA))
		{
			pDC->SetTextColor( RGB(255, 0, 0) );
		}
		else
		{
			pDC->SetTextColor( RGB(0, 0, 255) );  
		}		

		pDC->DrawText(strText, -1, &rect, DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_NOPREFIX);

		if(pOldFont != NULL)
		{
			pDC->SelectObject(pOldFont);
		}
	}

	pDC->SetBkMode(nOldMode);
	//ReleaseDC(pDC);
	// TODO:  Add your code to draw the specified item
}

BEGIN_MESSAGE_MAP(CListBoxKeyboard, CListBox)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

int CListBoxKeyboard::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListBox::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  Add your specialized creation code here

	return 0;
}

void CListBoxKeyboard::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	CFont* pFont = GetFont();
	CClientDC dc(this);
	CFont* pOldFont = NULL;
	if(pFont != NULL)
	{
		pOldFont = dc.SelectObject(pFont);	
	}	
	TEXTMETRIC tm;
	dc.GetTextMetrics(&tm);	
	if(pOldFont != NULL)
	{
		dc.SelectObject(pOldFont);
	}

	lpMeasureItemStruct->itemHeight = tm.tmHeight + tm.tmExternalLeading;
	// TODO:  Add your code to determine the size of specified item
}


void CListBoxKeyboard::OnPaint()
{
	CListBox::OnPaint();
}

BOOL CListBoxKeyboard::OnEraseBkgnd(CDC* pDC)
{
	return TRUE;
}

BOOL CKeyboardDataList::OnEraseBkgnd(CDC* pDC)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rcClient;
	GetClientRect(rcClient);

	CDC memDC; // memory device context.
	COLORREF clr = RGB(255,255,255);//dc.GetBkColor();//::GetSysColor(COLOR_3DSHADOW);
	// Create the memory DC, set Map Mode
	memDC.CreateCompatibleDC(pDC);
	memDC.SetMapMode(pDC->GetMapMode());

	// Create a bitmap big enough to hold the window's image
	CBitmap bitmap;
	bitmap.CreateCompatibleBitmap(pDC, rcClient.Width(), rcClient.Height());

	// Select the bitmap into the memory DC
	CBitmap* pOldBitmap = memDC.SelectObject(&bitmap);

	// Repaint the background, this takes the place of WM_ERASEBKGND.
	memDC.FillSolidRect(rcClient, clr);

	// Let the window do its default painting...
	CWnd::DefWindowProc( WM_PAINT, (WPARAM)memDC.m_hDC, 0 );

	// Blt it
	pDC->BitBlt(rcClient.left, rcClient.top, rcClient.Width(), rcClient.Height(),
		&memDC, 0, 0, SRCCOPY);            

	// Select the original bitmap back in
	memDC.SelectObject(pOldBitmap);

	// Clean up
	bitmap.DeleteObject();
	memDC.DeleteDC();
	return TRUE;
}

BOOL CKeyboardDataList::HSDataSourceSink_OnCommNotify( void* pData )
{
	if ( pData == NULL )
		return FALSE;
	CommNotify* pNotify = (CommNotify*)pData;
	if ( pNotify->m_uType == eDataSource_Init )
	{	
		//PostMessage(MsgInitStocks,0,0);
		m_ayStockInfo.RemoveAll();
		GetStocks();
	}
	return TRUE;
}

BOOL CKeyboardDataList::HSDataSourceSink_OnRecvData( void* pData, int nLen )
{
	return TRUE;
}

void CKeyboardDataList::GetStocks()
{
	for ( int i=0; i<m_ayStockPos.GetCount(); i++)
	{
		HSStockPosition* pPos = m_ayStockPos.GetAt(i);
		if ( pPos )
		{
			pPos->m_psiInfo->RemoveAll();
			delete pPos->m_psiInfo;
		}
		delete pPos;
	}
	m_ayStockPos.RemoveAll();
	for (int i=0; i<m_ayOrder.GetCount(); i++)
	{
		int mardetType;
		char *str;
		CodeInfo info;
		CString strOrder = m_ayOrder.GetAt(i);
		if ( !strOrder.IsEmpty() )
		{
			HSStockPosition* pPos = new HSStockPosition;
			pPos->m_psiInfo = new CArray<StockUserInfo>;
			CString strText;
			strText.Format("0x%s",strOrder);
			mardetType =  (int)strtol(strText.GetBuffer(), &str, 16);
			pPos->m_lBegin = 0;
			pPos->m_market = mardetType;
			int count = 0;
			if ( m_pDataSource )
				count = m_pDataSource->HSDataSourceEx_GetMarketStocksInfo(mardetType, *(pPos->m_psiInfo));
			pPos->m_lEnd = count;
			m_ayStockPos.Add(pPos);
		}
	}
}

LRESULT CKeyboardDataList::OnInitStocks(WPARAM wParam, LPARAM lParam)
{	
	m_ayStockInfo.RemoveAll();
	GetStocks();
	return 0;
}