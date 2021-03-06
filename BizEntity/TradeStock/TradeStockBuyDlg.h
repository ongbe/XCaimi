/*******************************************************
  源程序名称:TradeStockBuyDlg.h
  软件著作权:恒生电子股份有限公司
  系统名称:  投资赢家金融理财终端V1.0
  模块名称:  交易
  功能说明:  交易证券买入
  作    者:  shenglq
  开发日期:  20100903
*********************************************************/
#pragma once
#include "TradeStockBaseDlg.h"

class CTradeStockBuyDlg : public CTradeStockBaseDlg
{
	DECLARE_DYNAMIC(CTradeStockBuyDlg)
public:
	CTradeStockBuyDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CTradeStockBuyDlg();
	virtual void TradingAsyncCallBack(ITrading* pTrading, int iAction);
protected:	
	int m_nQueryEnableAction;
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInit();
	virtual void OnStockCodeExit();
	virtual void OnEntrustPriceExit();
	virtual void DoEntrust();
	virtual void LoadWnd();
	void QueryEnableBalance();
	void QueryEnableAmount();
private:
	DECLARE_MESSAGE_MAP()

};