/*******************************************************
  源程序名称:TradeFundAllotDlg.h
  软件著作权:恒生电子股份有限公司
  系统名称:  投资赢家金融理财终端V1.0
  模块名称:  交易
  功能说明:  基金申购
  作    者:  shenglq
  开发日期:  20110216
*********************************************************/
#pragma once
#include "TradeFundBaseDlg.h"

class CTradeFundAllotDlg : public CTradeFundBaseDlg
{
	DECLARE_DYNAMIC(CTradeFundAllotDlg)
public:
	CTradeFundAllotDlg(CWnd* pParent = NULL);   // 标准构造函数
	virtual ~CTradeFundAllotDlg();
protected:	
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持
	virtual BOOL OnInit();
	virtual void OnFundCodeExit();
	virtual void DoEntrust();
	virtual void LoadWnd();
	virtual BOOL Validate();

	DECLARE_MESSAGE_MAP()

};