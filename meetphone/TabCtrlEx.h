#pragma once


// CTabCtrlEx

class   CTabCtrlEx   :   public   CTabCtrl 
{ 
	//   Construction/destruction 
public: 
	CTabCtrlEx(); 
	virtual   ~CTabCtrlEx(); 

	//   Attributes: 
public: 

	//   Operations 
public: 
	void   SetColours(COLORREF   bSelColour,   COLORREF   bUnselColour); 

	//   Overrides 
	//   ClassWizard   generated   virtual   function   overrides 
	//{{AFX_VIRTUAL(CTabCtrlEx) 
public: 
	virtual   void   DrawItem(LPDRAWITEMSTRUCT   lpDrawItemStruct); 
protected: 
	virtual   void   PreSubclassWindow(); 
	//}}AFX_VIRTUAL 

	//   Implementation 
protected: 
	COLORREF   m_crSelColour,   m_crUnselColour; 

	//   Generated   message   map   functions 
protected: 
	//{{AFX_MSG(CTabCtrlEx) 
	afx_msg   int   OnCreate(LPCREATESTRUCT   lpCreateStruct); 
	//}}AFX_MSG 
	DECLARE_MESSAGE_MAP() 
};

