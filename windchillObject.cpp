// WindChillObject.cpp : CWindChillObject ��ʵ��

#include "stdafx.h"
#include <direct.h>
#include <afxinet.h>

#include "APSProp.h"
#include "Solid.h"
#include "Actionbase.h"
#include "AsmStep.h"
#include "CProduct.h"
#include "apsprocess.h"
#include "Equips.h"
#include "APSModel.h"

#include "tinyxml.h"
#include "common.h"
#include "DlgLogIn.h"
#include "DlgPBOMEdit.h"
#include "DlgMaterialEdit.h"
#include "WindChill_i.h"
#include "WindChillObject.h"
#include "WindChillSetting.h"
#include "KmZipUtil.h"
#include "UtilityFunction.h"
#include "WindChillXml.h"
#include "DlgSavePdf.h"
#include "DlgProcEdit.h"
#include "AsmInst.h"
#include "PBOMUtil.h"
#include "APSWaitCursor.h"
#include "DlgProcRule.h"
#include "EquipActionIn.h"
#include "Equips.h"
#include"APSProp.h"
#include "DlgAddMaterial.h"
#include "DlgPDFExport.h"
#include "DlgPDFConfig.h"
#include "Partinfo.h"
#include "PBomCreate.h"
#include "PBOMUtil.h"



using namespace std;
#define  MSG_FILE_NEW WM_USER + 2089
#define  MSG_UPDATE_PART WM_USER + 2091

bool g_islogin = false;   //


STDMETHODIMP CwindchillObject::InterfaceSupportsErrorInfo(REFIID riid)
{
	static const IID* const arr[] =
	{
		&IID_IWindChillObject
	};

	for (int i = 0; i < sizeof(arr) / sizeof(arr[0]); i++)
	{
		if (InlineIsEqualGUID(*arr[i], riid))
			return S_OK;
	}
	return S_FALSE;
}
STDMETHODIMP CwindchillObject::raw_IsChangeGetDownFileMode(void)
{
	return S_OK;
}


STDMETHODIMP CwindchillObject::raw_GetDownFilePath(long* strkey, BSTR* bstrFile)
{
	//��dll�е��öԻ���
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	CDlgAddMaterial dlg(ResourceType_EQUIPTOOL);
	if (dlg.DoModal() == IDOK)
	{
		auto contentXml = dlg.GetXMl();
		TiXmlPrinter printer;
		contentXml.Accept(&printer);
		CString str = "";
		str.Format("%s", printer.CStr());

		auto path = CWindChillXml::GetPath(str);

		path = "/notconvert";
		CString strFTPPath="notconvert", strFTPName = "1_gleich_winkel.prt", strModelName = "";

		//��ȡxml��ȡ���ļ��������Ʒ����ģ���ļ�
		//ftp����. 

		strFTPPath = path;

		if (!m_FTPInterface.Connect(CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd()))
		{
			MessageBox(NULL, "ftp����ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
			return False;
		}

		TCHAR TempDir[1024];
		GetTempPath(1024, TempDir);
		CString sTempDir(TempDir);
		CString tempdir = sTempDir + _T("KM3DCAPP-AWORK");

		if (!ExistDir(tempdir))
		{
			CreateDir(tempdir);
		}
		sTempDir = sTempDir + _T("KM3DCAPP-AWORK\\");
		CString strlocal = sTempDir + strFTPName;

		BOOL bSucc = m_FTPInterface.DownLoad(strlocal, CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), strFTPPath, strFTPName, CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd());
		if (!bSucc)
		{
			CString strErrMsg = _T("�����ļ�ʧ�ܣ�");
			//strErrMsg.Format(_T("�����ļ���%s��ʧ�ܣ�"), strFTPName);
			MessageBox(NULL, strErrMsg, APS_MSGBOX_TITIE, MB_OK);

			return FALSE;
		}

		*bstrFile = (_bstr_t)strlocal;

		return S_OK;
	}

	return S_FALSE;
}



STDMETHODIMP CwindchillObject::raw_AddOrUpdateSolid(UINT ID, BSTR bstrFile)
{
	IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();
	if (!doc)
	{
		ASSERT(FALSE);
		return S_FALSE;
	}
	//AfxMessageBox(CString(bstrFile));
	CAPSModel *pModel = (CAPSModel*)doc->GetAPSModel();

	auto Product = pModel->GetProduct();
	if (Product)
	{
		CSolid* Solid = Product->LookupSolid(ID);

		if (Solid)
		{
			
			for (int i = 0; i < CWindChillSetting::m_strRelMatch.GetSize(); ++i)
			{
				CString strName = CWindChillSetting::m_strRelMatch.GetAt(i).m_strName;
				CString strValue = CWindChillSetting::m_strRelMatch.GetAt(i).m_strValue;
				bool bShow = 1;
				bool bEdit = 0;

				CAPSProp Baseprop(strName, "", strName, 1, 1);

				Solid->UpdateProp(Baseprop.GetName(), CString(""));
				Solid->CSolid::ModifyPropStateByDisName(Baseprop.GetName(), 1, 1);
			}

			CAPSProp prop("#solid_add_file", "", "addFile", 1, 0);
			Solid->UpdateProp(prop.GetName(), CString(bstrFile));
			Solid->CSolid::ModifyPropStateByDisName(prop.GetName(), 0, 1);
		}
	}
	CWindChillSetting::GYJID.push_back(to_string((long long)(ID)));
	return S_OK;
}

STDMETHODIMP CwindchillObject::raw_GetName(BSTR* bstrName)
{
	*bstrName = SysAllocString(L"WindChill");

	return S_OK;
}

STDMETHODIMP CwindchillObject::raw_GetComment(BSTR* bstrName)
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::raw_Connect(IDispatch* app)
{
	HRESULT hr = app->QueryInterface(__uuidof(IAPSApplication), (void**)&m_pApplication);
	if (m_pApplication)
	{
		DWORD dwCookie = 0;
		hr = DispEventAdvise(app);
		//return hr;
	}
	hr = app->QueryInterface(__uuidof(IPSManager), (void**)&m_pPSM);
	if (m_pPSM)
	{
		DWORD dwCookie = 0;
		hr = DispEventAdvise(app);
		return hr;
	}

	return S_FALSE;
}

STDMETHODIMP CwindchillObject::raw_Disconnect()
{
	if (m_pApplication)
	{
		HRESULT hr = DispEventUnadvise(m_pApplication);
		m_pApplication->Release();
		m_pApplication = NULL;
		//return hr;
	}

	if (m_pPSM)
	{
		HRESULT hr = DispEventUnadvise(m_pPSM);
		m_pPSM->Release();
		m_pPSM = NULL;

		return hr;
	}

	return S_FALSE;
}

STDMETHODIMP CwindchillObject::raw_GetEnable(long bEnable)
{
	return S_OK;
}
////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CwindchillObject::OnAppInit(IAPSApplication* pdispApp)
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::OnAppClose()
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::OnDocumentOpening(BSTR bstrPath)
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::OnDocumentOpened(IAPSDocument* doc)
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::OnDocumentSaving(IAPSDocument* doc)
{
	return S_OK;
}

//���xml�ļ�
STDMETHODIMP CwindchillObject::OnDocumentSaved(IAPSDocument* doc, BSTR bstrPath)
{
	CAPSModel* pModel = (CAPSModel*)doc->GetAPSModel();
	return S_OK;
}


int CwindchillObject::OutputPDF(CString sPathFile, int iType, CString &sOutputFilePath)
{
	CString strlocal;
	BOOL bRet = GetCurFileName(m_pModel, true, strlocal);
	if (!bRet || strlocal.IsEmpty())
	{
		return FALSE;
	}

	CString strDllPath = GetModuleDir();
	strDllPath += _T("\\Output3DPdf.dll");
	HMODULE hLib = LoadLibrary(strDllPath);
	if (hLib)
	{
		typedef BOOL(WINAPI *InitA3DPDF)();

		InitA3DPDF pInitA3DPDF = (InitA3DPDF)GetProcAddress(hLib, _T("InitA3DPDF"));
		if (pInitA3DPDF)
		{
			pInitA3DPDF();
		}

		typedef BOOL(WINAPI *OutPutPdf)(CAPSModel*, LPCTSTR, LPCTSTR, LPCTSTR, bool, int, int, int&);

		OutPutPdf pOutPutPdf = (OutPutPdf)GetProcAddress(hLib, _T("OutPutPdf"));

		if (pOutPutPdf == NULL)
		{
			FreeLibrary(hLib);
			hLib = NULL;

			MessageBox(NULL, "δ�ܵ���PDF,����Output3DPdf.dll�Ƿ����.\n", APS_MSGBOX_TITIE, MB_OK);
			return FALSE;
		}

		while (1)
		{
			CDlgPDFConfig dlgConfig;
			dlgConfig.setTemplateName(sPathFile);
			dlgConfig.SetProcess(m_pProcess);
			dlgConfig.setTemplateType(iType);
			if (dlgConfig.DoModal())
			{

			}
			int iMode = dlgConfig.getMode();
			if (iMode == 0)//ȡ��
			{
				return 0;
			}
			if (iMode == 2)//��һ��
			{
				return 2;
			}

			CDlgPDFExport dlgExport;
			dlgExport.setFilePath(strlocal);
			if (dlgExport.DoModal())
			{

			}

			iMode = dlgExport.getMode();
			if (iMode == 2)//��һ��
			{
				continue;
			}
			else if (iMode == 1)//���
			{
				dlgExport.getOutputFilePath(sOutputFilePath);
				break;
			}
			else//ȡ��
			{
				return 0;
			}

			dlgExport.getOutputFilePath(sOutputFilePath);
			break;
		}

		if (sOutputFilePath.IsEmpty() || sPathFile.IsEmpty())
			return 0;

		CApsWaitCur waiter;

		int nCodeError = 0;

		BOOL bOutput = pOutPutPdf(m_pModel, sPathFile, "", sOutputFilePath, true, 1, iType, nCodeError);
		if (!bOutput)
		{
			MessageBox(NULL, "PDF����ʧ��.\n", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
			return FALSE;
		}
		else
		{
			CWindChillSetting::SetPdfPath_Rule(sOutputFilePath);
		}

		FreeLibrary(hLib);
		hLib = NULL;
	}
	else
	{
		MessageBox(NULL, "δ�ܵ���PDF,����Output3DPdf.dll�Ƿ����.\n", APS_MSGBOX_TITIE, MB_OK);
		return 0;
	}

	return 1;
}

CString CwindchillObject::GetModuleDir()
{
	HINSTANCE hModule = AfxGetInstanceHandleHelper();
	if (!hModule)
	{
		hModule = GetModuleHandle(_T("APSModelView.dll"));
	}

	TCHAR lpBuffer[MAX_PATH];
	ZeroMemory(lpBuffer, sizeof(lpBuffer));
	GetModuleFileName(hModule, lpBuffer, MAX_PATH);
	ASSERT(_tcslen(lpBuffer) > 0);

	CString strPath(lpBuffer);

	int nIndex = strPath.ReverseFind(_T('\\'));
	ASSERT(nIndex != -1);

	return strPath.Mid(0, nIndex);
}


void CwindchillObject::GetXMLConfig(std::map<int, CString> &mapConfig)
{
	mapConfig = m_mapConfig;
}

void CwindchillObject::ClearXMLConfig()
{
	m_mapConfig.erase(m_mapConfig.begin(), m_mapConfig.end());
	m_mapConfig.clear();
}

void CwindchillObject::SetParam(CAPSModel* pModel, CProcess* pProcess, CString sCfgPath, CString sFilePath)
{
	m_pModel = pModel;
	m_pProcess = pProcess;
	m_sCfgPath = sCfgPath;
	m_sOutPath = sFilePath;
	m_iPageNums = 0;
}

BOOL CwindchillObject::OpenPdfEditPage(CString sPathFile, CAPSModel* pModel, int iType)
{
	CString sOutputFilePath = "", sLoadFilePath = "";
	CDlgProcRule dlg;
	dlg.setTemplateName(sPathFile);
	dlg.SetProcess(m_pProcess);
	CString strlocal = "";
	BOOL bRet = GetCurFileName(pModel, true, strlocal);
	if (!bRet || strlocal.IsEmpty())
	{
		return FALSE;
	}

	//begin add by xjt in 2019.2.19 for 70780
	//������¼�ϴδ򿪵��ļ�·��
	static CString path = "";

	if (path == "")
	{
		dlg.setFilePath(strlocal);
	}
	else
	{
		dlg.setFilePath(strlocal);
	}

	//end add
	//if (pModel->GetOpenedFileName().IsEmpty())

	dlg.SetProcTempConfig(m_PdfConfigDatas);
	dlg.setTemplateType(iType);

	CApsWaitCur waiter;
	CString strDllPath = GetModuleDir();
	strDllPath += _T("\\Output3DPdf.dll");
	HMODULE hLib = LoadLibrary(strDllPath);
	if (hLib)
	{
		typedef BOOL(WINAPI *InitA3DPDF)();

		InitA3DPDF pInitA3DPDF = (InitA3DPDF)GetProcAddress(hLib, _T("InitA3DPDF"));
		if (pInitA3DPDF)
		{
			pInitA3DPDF();
		}

		if (dlg.DoModal())
		{
			dlg.getPath(sOutputFilePath, sLoadFilePath);
			path = sOutputFilePath;
			m_ConfigProcPDFName->clear();
			m_PdfConfigDatas->RemoveAll();
			dlg.GetProcTempConfig(m_PdfConfigDatas);
		}

		if (sOutputFilePath.IsEmpty() || sLoadFilePath.IsEmpty())
			return FALSE;

		SetParam(pModel, m_pProcess, sLoadFilePath, sOutputFilePath);

		CApsWaitCur waiter1;
		typedef BOOL(WINAPI *OutPutPdf)(CAPSModel*, LPCTSTR, LPCTSTR, LPCTSTR, bool, int, int, int&);

		OutPutPdf pOutPutPdf = (OutPutPdf)GetProcAddress(hLib, _T("OutPutPdf"));

		if (pOutPutPdf == NULL)
		{
			FreeLibrary(hLib);
			hLib = NULL;

			//KmMessageBox(ResourceString(IDS_APM_OUTPUT3D_INVALID, "����Output3DPdf.dll�Ƿ����.\n"), ResourceString(IDS_APM_TIPS, APS_MSGBOX_TITIE), MB_OK | MB_ICONERROR);
			//AfxMessageBox("����Output3DPdf.dll�Ƿ����.\n");
			MessageBox(NULL, "����Output3DPdf.dll�Ƿ����.\n", APS_MSGBOX_TITIE, MB_OK);
			return FALSE;
		}

		int nCodeError = 0;

		IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();
		if (!doc)
		{
			ASSERT(FALSE);
			return FALSE;
		}

		CAPSModel *pModel1 = (CAPSModel*)doc->GetAPSModel();
		BOOL bOutput = pOutPutPdf(pModel1, sLoadFilePath, "", sOutputFilePath, true, 1, iType, nCodeError);
		if (!bOutput)
		{
			/*switch (nCodeError)
			{
			case IDS_APM_TEMPLATE_ANIMATION3D:
			KmMessageBox(ResourceString(IDS_APM_TEMPLATE_ANIMATION3D, _T("XML����")),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			break;
			case IDS_APM_XMLTEMPLATE_ERROR:
			KmMessageBox(ResourceString(IDS_APM_XMLTEMPLATE_ERROR, _T("XML��ʽ")),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			break;
			case IDS_APM_TEMPLATE:
			KmMessageBox(ResourceString(IDS_APM_TEMPLATE, _T("XML����")),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			break;
			case IDS_LOAD_MODEL_ERROR:
			KmMessageBox(ResourceString(IDS_LOAD_MODEL_ERROR, _T("����ģ��")),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			break;
			case IDS_APM_PUBLISH_INITFAIL:
			KmMessageBox(ResourceString(IDS_APM_PUBLISH_INITFAIL, _T("��ʼ��hoops_publishʧ��!")),APS_MSGBOX_TITIE,MB_ICONERROR);
			break;
			case IDS_APM_VCLIB_ERROR:
			KmMessageBox(ResourceString(IDS_APM_VCLIB_ERROR, _T("�밲װ����ʱ��!")),APS_MSGBOX_TITIE,MB_ICONWARNING);
			break;
			case IDS_APM_PUBLISH_DIR_ERROR:
			KmMessageBox(ResourceString(IDS_APM_PUBLISH_DIR_ERROR, _T("hoops_publish��Ӧ��Ŀ¼����������ӦĿ¼!")),APS_MSGBOX_TITIE,MB_ICONERROR);
			break;
			case IDS_APM_PUBLISH_INVALID:
			KmMessageBox(ResourceString(IDS_APM_PUBLISH_INVALID, _T("hoops_publishע������ʧЧ.")),APS_MSGBOX_TITIE,MB_ICONERROR);
			break;
			default:
			KmMessageBox(ResourceString(IDS_APM_FAILEDOUTPUTPDF, _T("PDF���")),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			break;
			}*/
			//AfxMessageBox("����ʧ��.\n");
			MessageBox(NULL, "����ʧ��.\n", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
			return FALSE;
		}
		else
		{
			//KmMessageBox(ResourceString(IDS_APM_OUTPUTFINISHMSG, "������!"),APS_MSGBOX_TITIE,MB_ICONINFORMATION);
			//AfxMessageBox("�������.\n");
			MessageBox(NULL, "�������.\n", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
			CWindChillSetting::SetPdfPath_Rule(sOutputFilePath);
		}

		FreeLibrary(hLib);
		hLib = NULL;
	}
	else
	{
		//CString ssInfo = ResourceString(IDS_APM_OUTPUT3D_INVALID, "����Output3DPdf.dll�Ƿ����.\n");
		//KmMessageBox(ssInfo,APS_MSGBOX_TITIE,MB_ICONINFORMATION);
		MessageBox(NULL, "����Output3DPdf.dll�Ƿ����.\n", APS_MSGBOX_TITIE, MB_OK);
		return FALSE;
	}

	return TRUE;
}


std::map<CString, CString> CwindchillObject::GetStepinfo(CAsmStep * step)
{
	map<CString, CString>result;
	//��������
	auto baselist = step->GetBasePropList();
	if (baselist)
	{
		POSITION pos = baselist->GetHeadPosition();
		auto ID = step->GetID();
		result["id"] = to_string(static_cast<_LONGLONG>(ID)).c_str();

		while (pos != NULL)
		{
			CAPSProp prop = baselist->GetNext(pos);
			CString sVal = prop.ConvertTo();

			auto Name = prop.GetName();
			if (Name.CompareNoCase("#prop_name") == 0)
			{
				Name = "name";
			}

			if (Name.CompareNoCase("#prop_asm_content") == 0)
			{
				Name = "content";
			}
			result[Name] = sVal;
		}
	}

	auto pids = step->GetConstObjs();
	vector<UINT> IDS;
	for (auto i = 0; i < pids->GetCount(); ++i)
	{
		int nId = pids->GetAt(i);
		IDS.push_back(nId);
	}

	result["predecessors"] = "";
	auto Product = m_pModel->GetProduct();
	for (auto ID = IDS.begin(); ID != IDS.end(); ++ID)
	{
		if (Product)
		{
			CSolid* Solid = Product->LookupSolid(*ID);
			if (!Solid)
			{
				//ǰ�ù���
				if (result["predecessors"].IsEmpty())
				{
					result["predecessors"] = to_string(static_cast<_LONGLONG>(*ID)).c_str();
				}
				else
					result["predecessors"] += (";" + to_string(static_cast<_LONGLONG>(*ID))).c_str();
			}

		}
	}
	return result;
}


std::map<CString, CString> CwindchillObject::GetUserStepinfo(CAsmStep * step)
{
	map<CString, CString>result;
	//�Զ�������

	auto UserList = step->GetUserPropList();

	auto pos = UserList->GetHeadPosition();
	while (pos != NULL)
	{
		CAPSProp prop = UserList->GetNext(pos);
		CString sVal = prop.ConvertTo();
		auto Name = prop.GetName();
		result[Name] = sVal;
	}

	return result;
}


std::vector<std::map<CString, CString>> CwindchillObject::GetIdinfo(CAsmStep * step)
{
	auto pids = step->GetConstObjs();
	vector<UINT> IDS;
	for (auto i = 0; i < pids->GetCount(); ++i)
	{
		int nId = pids->GetAt(i);
		IDS.push_back(nId);
	}
	vector<map<CString, CString>> container;
	auto Product = m_pModel->GetProduct();
	for (auto ID = IDS.begin(); ID != IDS.end(); ++ID)
	{
		if (Product)
		{
			map<CString, CString>result;
			CSolid* Solid = Product->LookupSolid(*ID);

			if (Solid)
			{
				POSITION pos = Solid->m_baseList.GetHeadPosition();
				while (pos != NULL)
				{
					CAPSProp prop = Solid->m_baseList.GetNext(pos);
					CString sVal = prop.ConvertTo();
					auto Name = prop.GetName();
					result[Name] = sVal;
				}

				pos = Solid->m_useList.GetHeadPosition();
				while (pos != NULL)
				{
					CAPSProp prop = Solid->m_baseList.GetNext(pos);
					CString sVal = prop.ConvertTo();
					auto Name = prop.GetName();
					result[Name] = sVal;
				}
				container.push_back(result);
			}
		}
	}
	return container;
}

std::vector<std::map<CString, CString>> CwindchillObject::GetSolidinfo(CAsmStep * step)
{
	auto pids = step->GetConstObjs();
	vector<UINT> IDS;
	for (auto i = 0; i < pids->GetCount(); ++i)
	{
		int nId = pids->GetAt(i);
		IDS.push_back(nId);
	}
	vector<map<CString, CString>> container;
	auto Product = m_pModel->GetProduct();
	for (auto ID = IDS.begin(); ID != IDS.end(); ++ID)
	{
		if (Product)
		{
			map<CString, CString>result;
			CSolid* Solid = Product->LookupSolid(*ID);
			if (Solid)
			{
				result["id"] = to_string(static_cast<_LONGLONG>(*ID)).c_str();

				if (result["id"].Left(1) == "2")
					result["type"] = "prt";
				else
					result["type"] = "asm";

				auto Name = Solid->GetShowName();
				result["name"] = Name;

				auto fileName = Solid->GetEntityName();

				if (!fileName.IsEmpty())
				{
					fileName = Solid->GetEntityName() + ".hsf";
				}

				result["fileName"] = fileName;

				container.push_back(result);
			}

		}
	}
	return container;
}

std::map<CString, std::vector<std::map<CString, CString>>>  CwindchillObject::GetGridData(CArray<CAPSGridPropData*>* actionGridDatas)
{
	std::map<CString, std::vector<std::map<CString, CString>>> content;
	if (!actionGridDatas)
	{
		return content;
	}

	int len = actionGridDatas->GetCount();
	for (int i = 0; i < len; ++i)
	{
		std::vector<std::map<CString, CString>> container;
		CAPSGridPropData* pData = actionGridDatas->GetAt(i);
		CString sGridName = pData->GetGridName();
		CArray<CAPSPropList, CAPSPropList>* val = actionGridDatas->GetAt(i)->GetPropDataAry();
		int row = val->GetCount();
		map<CString, CString>result;
		for (int j = 0; j < row; ++j)
		{
			auto col = val->GetAt(j).GetSize();

			POSITION pos = val->GetAt(j).GetHeadPosition();

			while (pos)
			{
				CAPSProp prop = val->GetAt(j).GetNext(pos);
				CString sVal = prop.ConvertTo();
				auto Name = prop.GetName();
				result[Name] = sVal;
			}
			container.push_back(result);
		}

		content[sGridName] = container;
	}

	return content;
}

std::vector<CSolid *> CwindchillObject::GetStepSolid(CAsmStep * step)
{
	std::vector<CSolid *> container;

	if (step)
	{
		CArray<CAsmStep *, CAsmStep *>* pSubSteps = step->GetSubSteps();
		for (int m = 0; m < pSubSteps->GetSize(); m++)
		{
			auto pStep = pSubSteps->GetAt(m);
			if (pStep)
			{
				std::map<CString, CString> result;
				CArray<CActionBase *, CActionBase *>* pActions = pStep->GetActions();
				for (int i = 0; i < pActions->GetSize(); i++)
				{
					CActionBase* psAct = pActions->GetAt(i);
					if (CEquipActionIn* pEquipAct = dynamic_cast<CEquipActionIn*>(psAct))
					{
						CArray<UINT, UINT> * pObjs = pEquipAct->GetObjs();
						for (int j = 0; j < pObjs->GetCount(); ++j)
						{
							int nId = pObjs->GetAt(j);
							CEquips* pEquipsUion = m_pModel->GetEquips();
							if (pEquipsUion)
							{
								CSolid* pSolid = pEquipsUion->FindEquipSolid(nId);
								if (pSolid)
								{
									container.push_back(pSolid);
								}
							}
						}
					}
				}
			}
		}
	}

	return container;
}

std::map<CAsmStep *,std::map<CString, CString>>  CwindchillObject::GetChildStepInfo(CAsmStep * step)
{
	std::map<CAsmStep *,std::map<CString, CString>> container;

	if (step)
	{
		CArray<CAsmStep *, CAsmStep *>* pSubSteps = step->GetSubSteps();

	  //�����µ������Ӳ��� 
		for (int m = 0; m < pSubSteps->GetSize(); m++)
		{
			auto pStep = pSubSteps->GetAt(m);
			if (pStep)
			{
				auto stepinfo =GetStepinfo(pStep);
				container[pStep]=stepinfo;
			}
		}
	}

	return container;
}

//��ȡ�Ӳ����µĹ���װ���
std::vector<std::map<CString, CString>> CwindchillObject::GetChildStepEquies(CAsmStep * pStep)
{
	std::vector<std::map<CString, CString>> container;
	if (pStep)
	{
		std::map<CString, CString> result;
		CArray<CActionBase *, CActionBase *>* pActions = pStep->GetActions();
		for (int i = 0; i < pActions->GetSize(); i++)
		{
			CActionBase* psAct = pActions->GetAt(i);
			if (CEquipActionIn* pEquipAct = dynamic_cast<CEquipActionIn*>(psAct))
			{
				CArray<UINT, UINT> * pObjs = pEquipAct->GetObjs();
				for (int j = 0; j < pObjs->GetCount(); ++j)
				{
					int nId = pObjs->GetAt(j);
					CEquips* pEquipsUion = m_pModel->GetEquips();
					if (pEquipsUion)
					{
						CSolid* pSolid = pEquipsUion->FindEquipSolid(nId);
						if (pSolid)
						{
							result["id"] = to_string(static_cast<_LONGLONG>(nId)).c_str();
							auto Name = pSolid->GetShowName();
							result["name"] = Name;

							auto fileName = pSolid->GetShowName() + ".hsf";

							result["fileName"] = fileName;

							POSITION pos = pSolid->m_baseList.GetHeadPosition();
							while (pos != NULL)
							{
								CAPSProp prop = pSolid->m_baseList.GetNext(pos);
								CString sVal = prop.ConvertTo();
								auto Name1 = prop.GetName();
								result[Name1] = sVal;
							}

							pos = pSolid->m_useList.GetHeadPosition();
							while (pos != NULL)
							{
								CAPSProp prop = pSolid->m_useList.GetNext(pos);
								CString sVal = prop.ConvertTo();
								auto Name1 = prop.GetName();
								result[Name1] = sVal;
							}

						}

						container.push_back(result);
					}
				}
			}
		}
	}
	
	

	return container;
}

void CwindchillObject::GenMbomXml(CString strNewSubPath)
{
	TiXmlDocument doc;

	TiXmlElement * KmAssemblyProcess = new TiXmlElement("KmAssemblyProcess");

	TiXmlElement * ProcInfo = new TiXmlElement("ProcInfo");
	if (m_pProcess)
	{
		std::map<CString, CString> baseprop;

		std::map<CString, CString> userprop;
		POSITION pos = m_pProcess->GetBasePropList()->GetHeadPosition();
		while (pos)
		{
			CAPSProp prop = m_pProcess->GetBasePropList()->GetNext(pos);
			CString sVal = prop.ConvertTo();
			auto Name = prop.GetName();

			if (Name.CompareNoCase("#prop_name") == 0)
			{
				Name = "name";
			}

			if (Name.CompareNoCase("#prop_asm_content") == 0)
			{
				Name = "content";
			}


			baseprop[Name] = sVal;
		}

		for (auto it = baseprop.begin(); it != baseprop.end();++it)
		{
			ProcInfo->SetAttribute(it->first,it->second);
		}
		ProcInfo->SetAttribute("number",CWindChillSetting::m_strpartFirstName);

		pos = m_pProcess->GetUserPropList()->GetHeadPosition();
		while (pos)
		{

			CAPSProp prop = m_pProcess->GetBasePropList()->GetNext(pos);
			CString sVal = prop.ConvertTo();
			auto Name = prop.GetName();
			userprop[Name] = sVal;
		}
		TiXmlElement * ExtPropList = new TiXmlElement("ExtPropList");
		for (auto it = userprop.begin(); it != userprop.end(); ++it)
		{
			TiXmlElement * ExtProp = new TiXmlElement("ExtProp");

			ExtProp->SetAttribute("name", it->first);
			ExtProp->SetAttribute("value", it->second);

			ExtPropList->LinkEndChild(ExtProp);
		}

		ProcInfo->LinkEndChild(ExtPropList);

		CArray<CAPSGridPropData*>* actionGridDatas = m_pProcess->GetProcessGridDatas();
		auto GridData = GetGridData(actionGridDatas);

		TiXmlElement * GridPropList = new TiXmlElement("GridPropList");
		for (auto x = GridData.begin(); x != GridData.end(); ++x)
		{
			TiXmlElement * GridProp = new TiXmlElement("GridProp");
			GridProp->SetAttribute("name", x->first);

			TiXmlElement * GridRecordList = new TiXmlElement("GridRecordList");
			for (auto Gridit = (x->second).begin(); Gridit != (x->second).end(); ++Gridit)
			{
				TiXmlElement * GridRecord = new TiXmlElement("GridRecord");
				for (auto it = (*Gridit).begin(); it != (*Gridit).end(); ++it)
				{
					TiXmlElement * GridCell = new TiXmlElement("GridCell");

					GridCell->SetAttribute("name", it->first);
					GridCell->SetAttribute("value", it->second);

					GridRecord->LinkEndChild(GridCell);
				}
				GridRecordList->LinkEndChild(GridRecord);
			}
			GridProp->LinkEndChild(GridRecordList);
			GridPropList->LinkEndChild(GridProp);
		}

		ProcInfo->LinkEndChild(GridPropList);

	}
	else
		return; 

	auto steps = m_pProcess->GetAsmSteps();
	auto size = steps->GetSize();
	for (auto count = 0; count < steps->GetSize(); ++count)
	{
		CAsmStep* pSimple = steps->GetAt(count);
		//��ȡ�ò�������Ϣ
		auto stepinfo = GetStepinfo(pSimple);
		TiXmlElement * ProcedureInfo = new TiXmlElement("ProcedureInfo");

		for (auto it = stepinfo.begin(); it!= stepinfo.end();++it)
		{
			ProcedureInfo->SetAttribute(it->first,it->second);
		}

		//��ȡ�ò��������е�װ����󣬼�װ�������Ϣ
		auto Solidinfo = GetSolidinfo(pSimple);

		auto IDinfo = GetIdinfo(pSimple);

		TiXmlElement * AsmObjList = new TiXmlElement("AsmObjList");

		for (auto itID = IDinfo.begin(), itSolid = Solidinfo.begin(); itID != IDinfo.end(); ++itID, ++itSolid)
		{
			TiXmlElement * AsmObj = new TiXmlElement("AsmObj");

			for (auto it = (*itSolid).begin(); it != (*itSolid).end(); ++it)
			{
				AsmObj->SetAttribute(it->first, it->second);
			}

			TiXmlElement * Params = new TiXmlElement("Params");
			for (auto it = (*itID).begin(); it != (*itID).end(); ++it)
			{
				TiXmlElement * Param = new TiXmlElement("Param");
				Param->SetAttribute("name", it->first);
				Param->SetAttribute("value", it->second);
				Params->LinkEndChild(Param);
			}
			AsmObj->LinkEndChild(Params);
			AsmObjList->LinkEndChild(AsmObj);
		}

		ProcedureInfo->LinkEndChild(AsmObjList);

		TiXmlElement * ExtPropList = new TiXmlElement("ExtPropList");

		//�Զ�����Ϣ
		auto UserStepinfo = GetUserStepinfo(pSimple);
		for (auto it = UserStepinfo.begin(); it != UserStepinfo.end(); ++it)
		{
			TiXmlElement * ExtProp = new TiXmlElement("ExtProp");

			ExtProp->SetAttribute("name", it->first);
			ExtProp->SetAttribute("value", it->second);

			ExtPropList->LinkEndChild(ExtProp);
		}

		ProcedureInfo->LinkEndChild(ExtPropList);

		TiXmlElement * GridPropList = new TiXmlElement("GridPropList");

		//��������

		auto actionGridDatas = pSimple->GetActionGridDatas();
		auto GridData = GetGridData(actionGridDatas);

		for (auto x = GridData.begin(); x != GridData.end();++x)
		{
			TiXmlElement * GridProp = new TiXmlElement("GridProp");
			GridProp->SetAttribute("name", x->first);

			TiXmlElement * GridRecordList = new TiXmlElement("GridRecordList");
			for (auto Gridit = (x->second).begin(); Gridit != (x->second).end(); ++Gridit)
			{
				TiXmlElement * GridRecord = new TiXmlElement("GridRecord");
				for (auto it = (*Gridit).begin(); it != (*Gridit).end(); ++it)
				{
					TiXmlElement * GridCell = new TiXmlElement("GridCell");

					GridCell->SetAttribute("name", it->first);
					GridCell->SetAttribute("value", it->second);

					GridRecord->LinkEndChild(GridCell);
				}
				GridRecordList->LinkEndChild(GridRecord);
			}
			GridProp->LinkEndChild(GridRecordList);
			GridPropList->LinkEndChild(GridProp);
		}

		ProcedureInfo->LinkEndChild(GridPropList);

		TiXmlElement * StepList = new TiXmlElement("StepList");

		
		auto chidstepinfo =GetChildStepInfo(pSimple);
		
		for (auto childstep =chidstepinfo.begin();childstep!=chidstepinfo.end();++childstep)
		{
			auto childstepinfo =childstep->second;
			TiXmlElement * StepInfo = new TiXmlElement("StepInfo");
			for (auto childstepattr =childstepinfo.begin();childstepattr!=childstepinfo.end();++childstepattr)
			{
				StepInfo->SetAttribute(childstepattr->first,childstepattr->second);
			}

			auto childAsmstep =childstep->first;


			auto  childAsmstepinfo=GetChildStepEquies(childAsmstep);

			for (auto it =childAsmstepinfo.begin();it!=childAsmstepinfo.end();++it)
			{
				TiXmlElement * AsmObj = new TiXmlElement("AsmObj");
				AsmObj->SetAttribute("fileName", (*it)["fileName"]);
				(*it).erase("fileName");
				AsmObj->SetAttribute("id", (*it)["id"]);
				(*it).erase("id");
				AsmObj->SetAttribute("name", (*it)["name"]);
				(*it).erase("name");

				TiXmlElement * Params = new TiXmlElement("Params");
				for (auto itParam = (*it).begin(); itParam != (*it).end();++itParam)
				{
					TiXmlElement * Param = new TiXmlElement("Param");
					Param->SetAttribute("name", itParam->first);
					Param->SetAttribute("value", itParam->second);
					Params->LinkEndChild(Param);
				}
				AsmObj->LinkEndChild(Params);

				StepInfo->LinkEndChild(AsmObj);
			}
			StepList->LinkEndChild(StepInfo);
			/*TiXmlElement * AsmObj = new TiXmlElement("AsmObj");
				AsmObj->SetAttribute("fileName", (*it)["fileName"]);
				(*it).erase("fileName");
				AsmObj->SetAttribute("id", (*it)["id"]);
				(*it).erase("id");
				AsmObj->SetAttribute("name", (*it)["name"]);
				(*it).erase("name");

				TiXmlElement * Params = new TiXmlElement("Params");
				for (auto itParam = (*it).begin(); itParam != (*it).end();++itParam)
				{
					TiXmlElement * Param = new TiXmlElement("Param");
					Param->SetAttribute("name", itParam->first);
					Param->SetAttribute("value", itParam->second);
					Params->LinkEndChild(Param);
				}
				AsmObj->LinkEndChild(Params);*/
		}
		
		ProcedureInfo->LinkEndChild(StepList);

		ProcInfo->LinkEndChild(ProcedureInfo);
			/*
			TiXmlElement * AsmObj = new TiXmlElement("AsmObj");
			AsmObj->SetAttribute("fileName", (*it)["fileName"]);
			(*it).erase("fileName");
			AsmObj->SetAttribute("id", (*it)["id"]);
			(*it).erase("id");
			AsmObj->SetAttribute("name", (*it)["name"]);
			(*it).erase("name");

			TiXmlElement * Params = new TiXmlElement("Params");
			for (auto itParam = (*it).begin(); itParam != (*it).end();++itParam)
			{
				TiXmlElement * Param = new TiXmlElement("Param");
				Param->SetAttribute("name", itParam->first);
				Param->SetAttribute("value", itParam->second);
				Params->LinkEndChild(Param);
			}
			AsmObj->LinkEndChild(Params);
			StepList->LinkEndChild(AsmObj);
		}

		ProcedureInfo->LinkEndChild(StepList);

		ProcInfo->LinkEndChild(ProcedureInfo);*/
	}

	KmAssemblyProcess->LinkEndChild(ProcInfo);
	doc.LinkEndChild(KmAssemblyProcess);

	doc.SaveFile(strNewSubPath+"mbom.xml");

	

	//���ֽ�xmlתutf-8
	documentAnsiToutf8(strNewSubPath+"mbom.xml");

}


void CwindchillObject::GenProcXml(CString strNewSubPath)
{
	//����3dgx�ļ�������0.3dgx
	//std::vector<CString> vctXmlName;
	CFileFind finder;
	BOOL working = finder.FindFile(strNewSubPath + "\\*.3dgx");
	while (working)
	{
		working = finder.FindNextFile();
		if (finder.IsDots())
		{
			continue;
		}
		if (finder.IsDirectory())
		{
			continue;
		}
		/*if (finder.GetFileName().CompareNoCase("0.3dgx") == 0)
		{
		continue;
		}*/
		else
		{
			//��ѹ*.3dgx
			KmZipUtil kmZipUtil;
			if (kmZipUtil.OpenZipFile(strNewSubPath + finder.GetFileName(), KmZipUtil::ZipOpenForUncompress))
			{
				if (!ExistDir(strNewSubPath))
				{
					if (!::CreateDirectory(strNewSubPath, NULL))
					{
					}
				}
				kmZipUtil.UnZipAll(strNewSubPath);
				kmZipUtil.CloseZipFile();
			}
		}
	}
	MSXML2::IXMLDOMDocumentPtr	pProcXmlDoc(__uuidof(MSXML2::DOMDocument60));
	if (pProcXmlDoc == NULL)
	{
		ASSERT(FALSE);
		return;
	}
	//���ڵ�
	MSXML2::IXMLDOMElementPtr  Root;
	pProcXmlDoc->raw_createElement((_bstr_t)(char*)"KmAssemblyProcess", &Root);
	pProcXmlDoc->raw_appendChild(Root, NULL);
	//����*.3dgxs
	working = finder.FindFile(strNewSubPath + "\\*.3dgxs");
	while (working)
	{
		working = finder.FindNextFile();
		if (finder.IsDots())
		{
			continue;
		}
		if (finder.IsDirectory())
		{
			continue;
		}
		/*if (finder.GetFileName().CompareNoCase("0.3dgxs") == 0)
		{
		continue;
		}*/
		else
		{
			//�ڵ�ǰ3dgxs���AsmObj�ڵ��������
			MSXML2::IXMLDOMDocumentPtr	pXmlDoc(__uuidof(MSXML2::DOMDocument60));
			pXmlDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
			if (!pXmlDoc)
			{
				ASSERT(FALSE);
				continue;
			}
			CString tmp = strNewSubPath + finder.GetFileName();
			if (pXmlDoc->load((LPCTSTR)(tmp)) == VARIANT_FALSE)
			{
				ASSERT(FALSE);
				continue;
			}

			// ��ȡXML�ĵ����ڵ�
			MSXML2::IXMLDOMNodeListPtr pGroupList = pXmlDoc->GetchildNodes();
			int a = pGroupList->Getlength();
			if (pGroupList->Getlength() != 2)
				continue;

			MSXML2::IXMLDOMNodePtr pRoot = pGroupList->Getitem(1);
			if (pRoot == NULL)
			{
				ASSERT(FALSE);
				continue;
			}
			TraverseAsmObj(pRoot, strNewSubPath);
			MSXML2::IXMLDOMNodePtr pProcedure = pRoot->GetfirstChild();
			Root->appendChild(pProcedure);
		}
	}
	pProcXmlDoc->save(_variant_t(strNewSubPath + "Proc.xml"));
}


void CwindchillObject::TraverseAsmObj(MSXML2::IXMLDOMNodePtr pProcedure, CString strNewSubPath)
{
	MSXML2::IXMLDOMNodeListPtr pGroupList = pProcedure->GetchildNodes();
	if (pGroupList->Getlength() == 0)
	{
		CString strName = pProcedure->GetnodeName();
		if (strName.CompareNoCase(_T("AsmObj")) != 0)
		{
			return;
		}
		//��PBOM.xmlȡ��ǰasmobj������
		MSXML2::IXMLDOMElementPtr pItem = pProcedure;
		CString strID = pItem->getAttribute(_T("id"));
		AttachAsmObjAttr(pProcedure, strID, strNewSubPath);

	}
	else
	{
		for (int i = 0; i < pGroupList->Getlength(); i++)
		{
			MSXML2::IXMLDOMNodePtr pSub = pGroupList->Getitem(i);
			TraverseAsmObj(pSub, strNewSubPath);
		}
	}
}

void CwindchillObject::AttachAsmObjAttr(MSXML2::IXMLDOMNodePtr pProcedure, CString strID, CString strNewSubPath)
{
	MSXML2::IXMLDOMDocumentPtr	pXmlDoc(__uuidof(MSXML2::DOMDocument60));
	pXmlDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));

	if (!pXmlDoc)
	{
		ASSERT(FALSE);
		return;
	}

	if (pXmlDoc->load((LPCTSTR)(strNewSubPath + "PBOM.xml")) == VARIANT_FALSE)
	{
		ASSERT(FALSE);
		return;
	}

	// ��ȡXML�ĵ����ڵ�

	MSXML2::IXMLDOMNodeListPtr pGroupList = pXmlDoc->GetchildNodes();
	if (pGroupList->Getlength() != 2)
		return;


	MSXML2::IXMLDOMNodePtr pGroup = pGroupList->Getitem(1);
	MSXML2::IXMLDOMNodeListPtr pNodeList = pGroup->GetchildNodes();
	for (int i = 0; i < pNodeList->Getlength(); i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
		CString strName = pNode->GetnodeName();
		if (strName.CompareNoCase("Node") != 0)
			continue;
		if (TraverseNodeParams(pProcedure, pNode, strID))
			break;
	}

}

BOOL CwindchillObject::TraverseNodeParams(MSXML2::IXMLDOMNodePtr pProcedure, MSXML2::IXMLDOMNodePtr pNode, CString strID)
{
	MSXML2::IXMLDOMElementPtr pItem = pNode;
	CString strType = pItem->getAttribute(_T("type"));
	CString strInsID = pItem->getAttribute(_T("instanceid"));
	CString strName = pItem->getAttribute(_T("name"));
	MSXML2::IXMLDOMNodeListPtr pParamList = pNode->GetchildNodes();
	if (strInsID.CompareNoCase(strID) == 0)
	{
		MSXML2::IXMLDOMElementPtr pDest = pProcedure;
		pDest->setAttribute("name", (LPCTSTR)strName);
		if (pParamList != NULL)
		{
			for (int i = 0; i < pParamList->Getlength(); i++)
			{
				MSXML2::IXMLDOMNodePtr pParam = pParamList->Getitem(i);
				CString strParam = pParam->GetnodeName();
				if (strParam.CompareNoCase("Params") != 0)
					continue;
				pProcedure->appendChild(pParam);
				break;
			}
		}
		return TRUE;
	}
	if (strType.CompareNoCase("asm") == 0)
	{
		MSXML2::IXMLDOMNodeListPtr pNodeList = pNode->GetchildNodes();
		for (int i = 0; i < pNodeList->Getlength(); i++)
		{
			MSXML2::IXMLDOMNodePtr pSubNode = pNodeList->Getitem(i);
			CString strName1 = pSubNode->GetnodeName();
			if (strName1.CompareNoCase("Node") != 0)
				continue;
			if (TraverseNodeParams(pProcedure, pSubNode, strID))
				break;
		}
	}
	return FALSE;
}


CString CwindchillObject::GetRelativeDir()
{
	SYSTEMTIME tmptime;
	GetLocalTime(&tmptime);

	CString sUid = GetUid();
	char sTime[32] = { 0 };
	sprintf_s(sTime, "%.4d%.2d%.2d%s/", tmptime.wYear, tmptime.wMonth, tmptime.wDay, sUid);
	return sTime;
}

CString CwindchillObject::GetUid()
{
	CString strTmp = "";
	GUID guid;
	CoInitialize(NULL);
	if (::CoCreateGuid(&guid) == S_OK)
	{
		strTmp.Format("%.8x", guid.Data4);
	}
	CoUninitialize();
	return strTmp;
}
extern vector<string> split(string str, string pattern);

void CwindchillObject::CheckinPBom(CString strlocal)
{
	if (!m_FTPInterface.Connect(CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd()))
	{
		MessageBox(NULL, "ftp����ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
		return;
	}

	auto s =GetRelativeDir().FindOneOf("//");
	CString strFileName =  GetRelativeDir().Left(s)+ ".zip";

	CString strDir = strlocal.Left(strlocal.ReverseFind(_T('\\')) + 1);
	CString strNewSubPath;

	if (!GenAlonePath(strDir, strNewSubPath))
	{
		//strErrorInfo = CString(ResourceString(IDS_KMAPSTOOL2_FAILEDCD, "����Ŀ¼ʧ�ܣ�"));
		//break;
		return;
	}

	//����·��
	if (!CreateDirectory(strNewSubPath, NULL))
	{
		//strErrorInfo = CString(ResourceString(IDS_KMAPSTOOL2_FAILEDCD, "����Ŀ¼ʧ�ܣ�"));
		//break;
		return;
	}

	//�ڴ˴��޸��ϴ���ftp�����·��
	CString relativpath = "checkin/Pbom/";
	CString strRelDir = relativpath;//+ GetRelativeDir();

	//���ռ�ģ�ͼ���

	KmZipUtil kmZipUtil;

	if (kmZipUtil.OpenZipFile(strlocal, KmZipUtil::ZipOpenForUncompress))
	{
		if (!ExistDir(strNewSubPath))
		{
			if (!::CreateDirectory(strNewSubPath, NULL))
			{
			}
		}
		kmZipUtil.UnZipAll(strNewSubPath);
		//kmZipUtil.CloseZipFile();
	}
	kmZipUtil.CloseZipFile();


	CString strXml = strNewSubPath + "PBOM.xml";

	MSXML2::IXMLDOMDocumentPtr	pXmlDoc(__uuidof(MSXML2::DOMDocument60));
	pXmlDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
	if (!pXmlDoc)
	{
		ASSERT(FALSE);
	}

	if (pXmlDoc->load((LPCTSTR)(strXml)) == VARIANT_FALSE)
	{
		ASSERT(FALSE);
		//return false;
		return;
	}

	MSXML2::IXMLDOMNodeListPtr pGroupList = pXmlDoc->GetchildNodes();
	if (pGroupList->Getlength() != 2)
		return;

	MSXML2::IXMLDOMNodePtr pGroup = pGroupList->Getitem(1);
	MSXML2::IXMLDOMNodeListPtr pNodeList = pGroup->GetchildNodes();
	std::vector<CString> Path;

	std::vector<MSXML2::IXMLDOMNodeListPtr> Nodelist;

	for (int i = 0; i < pNodeList->Getlength(); i++)
	{
		MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
		CString strName = pNode->GetnodeName();
		if (strName.CompareNoCase("Node") != 0)
			continue;
		CAstXml::TraverseAstXmlNodeParam(pXmlDoc, pNode, Path,Nodelist, TRUE);
	}


	//�����ӵ��㲿����Ҫ�ֶ���дһЩ������Ϣ����һЩ����Ϊ�������ԣ��������ļ����Ѿ���д�����û����д���ԣ�����ʾ
	//������������д��ɣ����ܼ��룻
	//��������У��
	for (auto it = Nodelist.begin(); it != Nodelist.end(); ++it)
	{
		auto pNodeList1 = *it;
		for (int i = 0; i < pNodeList1->Getlength(); i++)
		{
			MSXML2::IXMLDOMNodePtr pParam = pNodeList1->Getitem(i);
			MSXML2::IXMLDOMElementPtr pItem = pParam;
			if (pItem)
			{
				CString strAttrName = pItem->getAttribute(_T("name"));
				CString strAttrValue = pItem->getAttribute(_T("value"));

				for (auto j = 0; j < CWindChillSetting::m_strCheckPropMatch.GetCount(); ++j)
				{
					CString strTmp = CWindChillSetting::m_strCheckPropMatch.GetAt(j).m_strName;
					if (strAttrName.CompareNoCase(strTmp) == 0 && strAttrValue.IsEmpty())
					{
						CString Prompt = _T("\"") + strTmp + _T("\"") + _T("Ϊ������д����");

						MessageBox(NULL, Prompt, APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
						return;
					}
				}
			}
		}
	}

	//add by xjt in 2019.1.19	
	//ɾ��vector�е��ظ�Ԫ��
	for (auto it = Path.begin(); it != Path.end();)
	{
		auto it1 = find(Path.begin(), Path.end(), *it);
		if (it1 != it)
			it = Path.erase(it);
		else
			++it;
	}
	//end


	if(kmZipUtil.OpenZipFile(strlocal, KmZipUtil::ZipOpenForCompress))
	{
		if (Path.size() > 0)
		{
			//	kmZipUtil.AddDirInZip(_T("epmInfo"), KmZipUtil::ZipOpenForCompress);
			for (auto it = Path.begin(); it != Path.end(); it++)
			{
				kmZipUtil.ZipFile(it->GetBuffer(), NULL);
			}
		}
	}

	kmZipUtil.CloseZipFile();

	BOOL bSucc = m_FTPInterface.UpLoad(strlocal, CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), strRelDir, strFileName);
	if (!bSucc)
	{
		MessageBox(NULL, "�ϴ�ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
		return;
	}

	/*
	KmZipUtil kmZipUtil;

	if (kmZipUtil.OpenZipFile(strlocal, KmZipUtil::ZipOpenForUncompress))
	{
	if (!ExistDir(strNewSubPath))
	{
	if (!::CreateDirectory(strNewSubPath, NULL))
	{
	}
	}
	kmZipUtil.UnZipAll(strNewSubPath);
	//kmZipUtil.CloseZipFile();
	}

	kmZipUtil.CloseZipFile();

	CString strXml = strNewSubPath + "PBOM.xml";

	MSXML2::IXMLDOMDocumentPtr	pXmlDoc(__uuidof(MSXML2::DOMDocument60));
	pXmlDoc.CreateInstance(__uuidof(MSXML2::DOMDocument60));
	if (!pXmlDoc)
	{
	ASSERT(FALSE);
	}

	if (pXmlDoc->load((LPCTSTR)(strXml)) == VARIANT_FALSE)
	{
	ASSERT(FALSE);
	//return false;
	return;
	}

	MSXML2::IXMLDOMNodeListPtr pGroupList = pXmlDoc->GetchildNodes();
	if (pGroupList->Getlength() != 2)
	return;

	MSXML2::IXMLDOMNodePtr pGroup = pGroupList->Getitem(1);
	MSXML2::IXMLDOMNodeListPtr pNodeList = pGroup->GetchildNodes();
	std::vector<CString> Path;
	for (int i = 0; i < pNodeList->Getlength(); i++)
	{
	MSXML2::IXMLDOMNodePtr pNode = pNodeList->Getitem(i);
	CString strName = pNode->GetnodeName();
	if (strName.CompareNoCase("Node") != 0)
	continue;
	CAstXml::TraverseAstXmlNodeParam(pXmlDoc, pNode, Path, TRUE);
	}

	//add by xjt in 2019.1.19	
	//ɾ��vector�е��ظ�Ԫ��
	for (auto it = Path.begin(); it != Path.end();)
	{
	auto it1 = find(Path.begin(), Path.end(), *it);
	if (it1 != it)
	it = Path.erase(it);
	else
	++it;
	}
	//end

	KmZipUtil zip;
	auto temppath = strNewSubPath + "AddorUpdate.zip";
	if (Path.size() > 0)
	{
	if (zip.CreateZipFile(temppath))
	{
	for (auto it = Path.begin(); it != Path.end(); it++)
	{
	zip.ZipFile(it->GetBuffer(), NULL);
	}
	zip.CloseZipFile();
	}

	bSucc = m_FTPInterface.UpLoad(temppath, CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), strRelDir, "AddorUpdate.zip");
	if (!bSucc)
	{
	MessageBox(NULL, "�ϴ�ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
	return;
	}
	}

	*/

	CString strInput = "<?xml version = \"1.0\" encoding = \"UTF-8\"?><part><xmlFileName>PBOM.xml</xmlFileName><fileName>"
		+ strFileName + "</fileName>" +
		"<partNumber>" + CWindChillSetting::m_strpartFirstName + "</partNumber>" +
		"</part>";

	auto Xmlcontent = m_WebserverInterface.checkInPbom(strInput);
	bool bSucess = CWindChillXml::ParseResult(Xmlcontent);
	if (bSucess)
	{
		MessageBox(NULL, "����ɹ�", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
	}
	else
	{
		MessageBox(NULL, "����ʧ��", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST );
	}

	if (ExistDir(strNewSubPath)) 
	{
		DeleteDir(strNewSubPath);
	}
}


static CArray<CAsmStep*, CAsmStep*> *steps = NULL;


void CwindchillObject::UserPropIsEdit()
{
	IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();

	if (!doc)
	{
		//begin add by xjt in 2019.2.19 for 70785
		auto m_hwnd = AfxGetMainWnd()->m_hWnd;
		::SendMessage(m_hwnd, MSG_FILE_NEW, NULL, LPARAM(""));
		doc = m_pApplication->GetCurrentDocment();
		//end add
	}
	
	CAPSModel *pModel = (CAPSModel*)doc->GetAPSModel();
	
	auto product =pModel->GetProduct();

	for (int i = 0; i < CWindChillSetting::m_strRelMatch.GetSize(); ++i)
	{
		CString strName = CWindChillSetting::m_strRelMatch.GetAt(i).m_strName;
		bool bShow = 1;
		bool bEdit = 0;
		auto Process = pModel->GetProcess();
		CAsmInst *pAsmRoot = Process->GetAsmInst();

		if (pAsmRoot != NULL)
		{
			pAsmRoot->CAsmInst::ModifyPropStateByDisName(strName, bShow, bEdit);
		}
	}

	/*
	for (auto it = CWindChillSetting::PROCUDEID.begin(); it != CWindChillSetting::PROCUDEID.end(); ++it)
	{
		CSolid* Solid = product->LookupSolid(atoi(it->c_str()));

		if (Solid)
		{
			for (int i = 0; i < CWindChillSetting::m_strRelMatch.GetSize(); ++i)
			{
				CString strName = CWindChillSetting::m_strRelMatch.GetAt(i).m_strName;
				CString strValue = CWindChillSetting::m_strRelMatch.GetAt(i).m_strValue;
				
				bool bShow = 1;
				bool bEdit = 0;

				CAPSProp Baseprop(strName, "", strValue, 1, 0);
				
				Solid->SeekObjPropByDisName(strName, Baseprop);
				//Solid->UpdateProp(Baseprop.GetName(), CString("1"));
				Solid->CSolid::ModifyPropStateByDisName(Baseprop.GetName(), 1, 0);
			}
		}
	}

	*/
	for (auto it =CWindChillSetting::GYJID.begin();it!=CWindChillSetting::GYJID.end();++it)
	{
		CSolid* Solid = product->LookupSolid(atoi((*it).c_str()));

		if (Solid)
		{
			for (auto i = 0; i < CWindChillSetting::m_strUpdatePropMatch.GetCount() - 1; ++i)
			{
				CString strName = CWindChillSetting::m_strUpdatePropMatch.GetAt(i).m_strName;
				CString strValue = CWindChillSetting::m_strUpdatePropMatch.GetAt(i).m_strValue;

				CAPSProp Baseprop(strName, "", "", 1, 1);

				Solid->UpdateProp(Baseprop.GetName(), CString(""));
				Solid->CSolid::ModifyPropStateByDisName(Baseprop.GetName(), 1, 1);


				for (int i = 0; i < CWindChillSetting::m_strRelMatch.GetSize(); ++i)
				{
					CString strName = CWindChillSetting::m_strRelMatch.GetAt(i).m_strName;

					bool bShow = 1;
					bool bEdit = 1;
					//��windhcill�������Ĺ��ռ����� �� ��� �����޸�
					if (strName.CompareNoCase("���")==0)
					{
						Solid->CSolid::ModifyPropStateByDisName(strName, 1,0 );
					}
					else
					{
						Solid->CSolid::ModifyPropStateByDisName(strName, 1, 1);
					}
				}
			}
		}
	}

}
//add by xingjt for �����㲿��
BOOL CwindchillObject::UpdatePart()
{
	IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();

	//test();

	if (!doc)
	{
		//begin add by xjt in 2019.2.19 for 70785
		auto m_hwnd = AfxGetMainWnd()->m_hWnd;
		::SendMessage(m_hwnd, MSG_FILE_NEW, NULL, LPARAM(""));
		doc = m_pApplication->GetCurrentDocment();
		//end add
	}
	CAPSModel *pModel = (CAPSModel*)doc->GetAPSModel();

	auto array = pModel->GetSelects(1);

	for (auto i = 0; i < array->GetCount(); ++i)
	{
		auto ID = array->GetAt(i);

		auto Product = pModel->GetProduct();

		auto Solid = Product->LookupSolid(ID);
		if (Solid)
		{
			CString Type = "6";

			auto Name = Solid->GetShowName();

			auto  Xmlcontent = m_WebserverInterface.getDocInfoOfContent(Type, Name);

			if (Xmlcontent.IsEmpty())
			{
				return S_FALSE;
			}

			auto path = CWindChillXml::GetPath(Xmlcontent);
			//����ѹ��������  ��Ҫ��ftp����

			//CWindChillSetting::m_strpartFirstName = path;

			CString strFTPPath, strFTPName;
			//�����ļ���·��
			strFTPPath = "/downloadContent/";
			strFTPName = path;

			TCHAR TempDir[1024];
			GetTempPath(1024, TempDir);
			CString sTempDir(TempDir);
			sTempDir = sTempDir + _T("KM3DCAPP-AWORK\\");
			CString strlocal = sTempDir + strFTPName;
			if (!ExistDir(sTempDir))
			{
				if (!::CreateDirectory(sTempDir, NULL))
				{
				}
			}
			if (!m_FTPInterface.Connect(CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd()))
			{
				MessageBox(NULL, _T("ftp����ʧ�ܣ�"), APS_MSGBOX_TITIE, MB_OK);

				return S_FALSE;
			}

			BOOL bSucc = m_FTPInterface.DownLoad(strlocal, CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), strFTPPath, strFTPName, CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd());
			if (!bSucc)
			{
				CString strErrMsg = _T("�����ļ�ʧ�ܣ�");
				//strErrMsg.Format(_T("�����ļ���%s��ʧ�ܣ�"), strFTPName);
				MessageBox(NULL, strErrMsg, APS_MSGBOX_TITIE, MB_OK);
				return S_FALSE;
			}

			if (strFTPName.Find(_T(".zip")) >= 0)  //���ݵ�ѹ���ļ���
			{
				int nP = strFTPName.ReverseFind(_T('.'));
				CString strDir = sTempDir + strFTPName.Left(nP) + "\\";  //ѹ���ļ���ѹ����ļ���
				KmZipUtil kmZipUtil;
				if (kmZipUtil.OpenZipFile(strlocal, KmZipUtil::ZipOpenForUncompress))
				{
					if (!ExistDir(strDir))
					{
						if (!::CreateDirectory(strDir, NULL))
						{
						}
					}
					kmZipUtil.UnZipAll(strDir);
					kmZipUtil.CloseZipFile();
					//DeleteFile(strlocal);
					CFileFind find;


					auto testcontent = m_WebserverInterface.getBom("1", Name);

					TiXmlDocument xml;

					auto isopen = xml.Parse(testcontent);

					if (!isopen)
					{
						return S_FALSE;
					}

					std::vector<PartInfo> partInfo;
					TiXmlElement *root = xml.FirstChildElement();
					if (root == NULL || root->FirstChildElement() == NULL)
					{
						xml.Clear();
						return FALSE;
					}

					auto elem = root->FirstChildElement()->FirstChildElement();
					while (elem != NULL)
					{
						PartInfo part(elem);
						partInfo.push_back(part);
						elem = elem->NextSiblingElement();
					}

					auto  parttop = partInfo.begin();
					for (auto partitem = partInfo.begin(); partitem != partInfo.end(); ++partitem)
					{
						if (CString(partitem->value["Number"].c_str()).CompareNoCase(Name) == 0)
						{
							parttop = partitem;
						}
					}

					CString ProductPath = parttop->value["epmfilename"].c_str();
					ProductPath = "1.catpart";
					auto partdir = strDir + GetMainFileName(path) + _T("\\");
					auto isFind = find.FindFile(partdir + ProductPath);
					if (isFind)
					{
						isFind = find.FindNextFile();
						auto partpath = find.GetFilePath();
						auto m_hwnd = AfxGetMainWnd()->m_hWnd;

						CString strxml;
						if (!ConvertModel(partpath, strxml))
						{
							return FALSE;
						}
						
						if (PathFileExists(strxml))
						{

							if (CWindChillSetting::m_iConvertMode != 1)//exchange 
							{
								//������д��xml��
								CPBOMUtil util;

								util.CreatePbomExchange(strxml, testcontent, Name);
								documentAnsiToutf8(strxml);
							}
							else
							{
								CPBOMUtil util;
								//strXml = "C:\\Users\\Administrator\\Desktop\\39909B84-206E-4BE4-BD29-631CD9B81839\\D6101000.xml";

								util.CreatePbomIop(strxml, testcontent, Name);
								documentAnsiToutf8(strxml);
							}

							CStringArray value;
							value.Add(to_string(static_cast<long long>(ID)).c_str());
							value.Add(strxml);
							::SendMessage(m_hwnd, MSG_UPDATE_PART, NULL, LPARAM(&value));
						}
					}
				}
				else
				{
					MessageBox(NULL, _T("��ѹʧ�ܣ�"), APS_MSGBOX_TITIE, MB_OK);
					return S_FALSE;
				}
			}
		}
	}

	return TRUE;
}
//end add

void CwindchillObject::CheckinProc(CString kmapxpath, CString pdfpath)
{

LAST:
	CString strPDFName = "";
	//PDF�ļ�
	CString strOutPDF;

	CString TemplateAry = _T("PDFTemplate");
	CString sPath = "";
	sPath.Format(_T("%s%s%s"), GetModuleDir(), _T("\\Resources\\Chs\\"), TemplateAry);
	CString sPathFile = sPath;

	sPathFile.Append(_T("\\"));
	sPathFile.Append(_T("���չ��ģ��.xml"));

	CApsWaitCur waiter;
	int iRet = OutputPDF(sPathFile, 1, strOutPDF);
	
	if (0 == iRet)
	{
		return;
	}
	if (2 == iRet)//������һ��
	{
		goto LAST;
	}

	strPDFName = strOutPDF.Right(strOutPDF.GetLength() - strOutPDF.ReverseFind(_T('\\')) - 1);
	if (strPDFName.IsEmpty())
		return;


	pdfpath = strOutPDF;
	if (!ExistFile(kmapxpath))
	{
		MessageBox(NULL, "kmapx�ļ������ڣ�", APS_MSGBOX_TITIE, MB_OK);
		return;
	}

	auto dirty=m_pModel->m_bModify;
	//auto dirty =m_pModel->GetProduct()->IsModify();
	//auto dirty1 = m_pModel->GetProcess()->IsModify();

	if (!ExistFile(pdfpath))
	{
		MessageBox(NULL, "�����pdf�ļ���", APS_MSGBOX_TITIE, MB_OK);
		return;
	}

	if (!m_FTPInterface.Connect(CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd()))
	{
		MessageBox(NULL, "ftp����ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
		return;
	}

	CString strFileName = GetFileName(kmapxpath);

	CString strDir = kmapxpath.Left(kmapxpath.ReverseFind(_T('\\')) + 1);
	CString strNewSubPath;

	if (!GenAlonePath(strDir, strNewSubPath))
	{
		//strErrorInfo = CString(ResourceString(IDS_KMAPSTOOL2_FAILEDCD, "����Ŀ¼ʧ�ܣ�"));
		//break;
		return;
	}

	//����·��
	if (!CreateDirectory(strNewSubPath, NULL))
	{
		//strErrorInfo = CString(ResourceString(IDS_KMAPSTOOL2_FAILEDCD, "����Ŀ¼ʧ�ܣ�"));
		//break;
		return;
	}

	//�ڴ˴��޸��ϴ���ftp�����·��
	CString relativpath = "checkin/doc/";
	CString strRelDir = relativpath ;//+ GetRelativeDir();

	KmZipUtil kmZipUtil;
	if (kmZipUtil.OpenZipFile(kmapxpath, KmZipUtil::ZipOpenForUncompress))
	{
		if (!ExistDir(strNewSubPath))
		{
			if (!::CreateDirectory(strNewSubPath, NULL))
			{
			}
		}
		kmZipUtil.UnZipAll(strNewSubPath);
	}
	kmZipUtil.CloseZipFile();

	CString strXmlName = "";
	//��װ����xml������PBOM.xml
	GenProcXml(strNewSubPath);
	GenMbomXml(strNewSubPath);

	if (PathFileExists(strNewSubPath + "Proc.xml") && PathFileExists(strNewSubPath + "mbom.xml"))
	{
		KmZipUtil zip;
		CString zipPath = pdfpath.Left(pdfpath.GetLength() - 3) + "zip";
		if (zip.CreateZipFile(zipPath))
		{
			zip.ZipFile(kmapxpath, NULL);
			zip.ZipFile(pdfpath, NULL);
			zip.ZipFile(strNewSubPath + "Proc.xml", NULL);
			zip.ZipFile(strNewSubPath + "mbom.xml", NULL);

		}
		zip.CloseZipFile();
		strFileName = GetFileName(zipPath);
		BOOL bSucc = m_FTPInterface.UpLoad(zipPath, CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), strRelDir, strFileName);
		if (!bSucc)
		{
			MessageBox(NULL, "�ϴ�ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
			return;
		}

		CString strInput =  
			CString("<?xml version=\"1.0\" encoding=\"UTF-8\"?>")+
			"<checkInDoc>"+
			"<partNumber>"+CWindChillSetting::m_strpartFirstName+"</partNumber>"+
			"<fileName>"+GetFileName(zipPath)+"</fileName>"+
			"</checkInDoc>";

		auto Xmlcontent = m_WebserverInterface.CheckInDoc("4", strInput);
		bool bSucess = CWindChillXml::ParseResult(Xmlcontent);
		if (bSucess)
		{
			MessageBox(NULL, "����ɹ�", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST );
		}
		else
		{
			MessageBox(NULL, "����ʧ��", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST );
		}

		if (ExistDir(strNewSubPath))
		{
			DeleteDir(strNewSubPath);
		}
	}
}


void test()
{
	CFTPInterface m_FTPInterface;
	if (!m_FTPInterface.Connect(CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(), CWindChillSetting::GetStrFTPUserName(), CWindChillSetting::GetStrFTPPasswd()))
	{
		MessageBox(NULL, "ftp����ʧ�ܣ�", APS_MSGBOX_TITIE, MB_OK);
		return;
	}


auto	bSucc = m_FTPInterface.UpLoad("C:\\Users\\Administrator\\Desktop\\11111\\Product1.CATProduct", CWindChillSetting::GetStrFTPURL(), CWindChillSetting::GetStrFTPPort(),"/EPMDocumentInfo", "Product1.CATProduct");
	if (!bSucc)
	{
		CString strErrMsg = _T("�ϴ��ļ�ʧ�ܣ�");

		return ;
	}
}

STDMETHODIMP CwindchillObject::OnPluginCommand(long nID)
{
	IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();

	//test();

	if (!doc)
	{
		//begin add by xjt in 2019.2.19 for 70785
		auto m_hwnd = AfxGetMainWnd()->m_hWnd;
		::SendMessage(m_hwnd, MSG_FILE_NEW, NULL, LPARAM(""));
		doc = m_pApplication->GetCurrentDocment();
		//end add
	}
	CAPSModel *pModel = (CAPSModel*)doc->GetAPSModel();
	m_pProcess = pModel->GetCurApsProcess();

	AFX_MANAGE_STATE(AfxGetStaticModuleState())

		//CwindchillObjectXml("D:\\mbom\\���ٻ�.kmapx\\");
		if (nID == m_nLoginCmd)
		{
			CDlgLogIn dlg(g_islogin);
			dlg.DoModal();
			g_islogin = dlg.GetloginStae();
		}
		else if (nID == m_nPBomEditCmdl)
		{
			CDlgPBOMEdit dlg(pModel, QueryType_PBOM, m_pApplication);
			dlg.SetCaption("PBOM�༭��ѯ");
			dlg.DoModal();

			UserPropIsEdit();


		}
		else if (nID == m_nPBOMCmd)
		{
			CDlgPBOMEdit dlg(pModel, QueryType_EBOM, m_pApplication);
			dlg.SetCaption("PBOM������ѯ");
			dlg.DoModal();
			UserPropIsEdit();
		}
		else if (nID == m_nMaterialCmd)
		{
			CDlgMaterialEdit dlg(pModel);
			dlg.DoModal();
		}
		else if (nID == m_nCheckinPBomCmd)
		{
			auto pProduct = pModel->GetProduct();
			if (pProduct)
			{
				auto isEmpty = pProduct->GetIsEmpty();
				if (isEmpty)
				{
					MessageBox(NULL, "��ǰΪ���ĵ����������Ʒ����ļ���", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
					return S_OK;
				}
			}

			CString strlocal = "";
			auto bRet = GetCurFileName(pModel, true, strlocal);
			if (!bRet || strlocal.IsEmpty())
			{
				return S_OK;
			}
			CheckinPBom(strlocal);
		}
		else if (nID == m_CreatProcCmd)
		{
			CDlgPBOMEdit dlg(pModel, QueryType_RULE, m_pApplication);
			dlg.SetCaption("PBOM��ѯ");
			dlg.DoModal();

			UserPropIsEdit();
		}
		else if (nID == m_ProcQueryCmd)
		{
			CDlgProcEdit dlg;
			dlg.DoModal();
			
			UserPropIsEdit();
		}
		else if (nID == m_ProcCheckInCmd)
		{
			auto pProduct = pModel->GetProduct();
			if (pProduct)
			{
				auto isEmpty =pProduct->GetIsEmpty();
				if (isEmpty)
				{
					MessageBox(NULL, "��ǰΪ���ĵ����������Ʒ����ļ���", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
					return S_OK;
				}
			}


			CString strlocal = "";
			m_pModel = pModel;
			auto bRet = GetCurFileName(pModel, true, strlocal);
			if (!bRet || strlocal.IsEmpty())
			{
				return S_OK;
			}

			//�����ĵ�����Ϣ���ӵ�kmapx�ļ�
			if (m_pProcess)
			{	
				auto userlist = m_pProcess->GetUserPropList();
				for (auto it = CWindChillSetting::ReviewValue.begin(); it != CWindChillSetting::ReviewValue.end(); ++it)
				{
					CAPSProp propAdd;

					auto pos =userlist->SeekProp(it->first,&propAdd);
					if (pos)   //˵��������
					{
						propAdd.SetType(1);
						propAdd.ModifyByType(it->second);
						userlist->ModifyProp(it->first, propAdd);
					}
				}
			}

			CheckinProc(strlocal, CWindChillSetting::GetPdfPath_Rule());
		}
		else if (nID == m_ProcPdfEditCmd)
		{
			auto pProduct = pModel->GetProduct();
			if (pProduct)
			{
				auto isEmpty = pProduct->GetIsEmpty();
				if (isEmpty)
				{
					MessageBox(NULL, "��ǰΪ���ĵ����������Ʒ����ļ���", APS_MSGBOX_TITIE, MB_OK | MB_TOPMOST);
					return S_OK;
				}

			}

			CString TemplateAry = _T("PDFTemplate");
			CString sPath = _T("");//::GetConfigFileFullPath(TemplateAry);
			sPath.Format(_T("%s%s%s"), GetModuleDir(), _T("\\Resources\\Chs\\"), TemplateAry);
			CString sPathFile = sPath;
			sPathFile.Append(_T("\\"));
			sPathFile.Append(_T("���չ��ģ��.xml"));
			OpenPdfEditPage(sPathFile, pModel, 1);
		}
		else if (nID == m_ImportEquiesCmd)
		{
			auto array = pModel->GetSelects(2);
			for (auto i = 0; i < array->GetCount(); ++i)
			{
				auto ID = array->GetAt(i);

				CDlgAddMaterial dlg(ResourceType_EQUIPTOOL, ID);
				if (dlg.DoModal() == IDOK)
				{

				}
			}
		}
		else if (nID == m_UpdatePartCmd)
		{
			UpdatePart();

			UserPropIsEdit();
		}

		return S_OK;
}

STDMETHODIMP CwindchillObject::OnUpdatePluginCommandUI(long nID, long* state)
{
	//���ͬʱ���ض�����������������
	if (nID == m_nLoginCmd || nID == m_nPBOMCmd || nID == m_nPBomEditCmdl ||
		nID == m_nMaterialCmd || nID == m_nProcRuleCmd || nID == m_nCheckinPBomCmd ||
		nID == m_CreatProcCmd || nID == m_ProcQueryCmd || nID == m_ProcCheckInCmd ||
		nID == m_ProcPdfEditCmd ||nID == m_ImportEquiesCmd ||nID == m_UpdatePartCmd
		)
	{
		IAPSDocumentPtr doc = m_pApplication->GetCurrentDocment();
		if (!doc)
		{
			return S_OK;
		}

		CAPSModel *pModel = (CAPSModel*)doc->GetAPSModel();
	
		
		auto isshowImportEquies = false;
		auto isshowUpdatePart = false;

		auto arrayUpdatePart = pModel->GetSelects(1);
		auto arrayImportEquies = pModel->GetSelects(2);
		//auto arrayImportEquies = m_pProcess->GetCurStep();

		if (arrayUpdatePart->GetCount() > 0)
		{
			isshowUpdatePart = true;
		}
		/*
		if (arrayImportEquies->GetCount() > 0)
		{
			auto ID = arrayImportEquies->GetAt(0);
			if (IsAsmStep(ID))
			{
				if (m_pProcess)
				{
					auto step = m_pProcess->GetCurStep();

					if (step)
					{
						CArray<CAsmStep *, CAsmStep *>* pSubSteps = step->GetSubSteps();
						if (pSubSteps->GetSize() == 0)
						{
							isshowImportEquies = true;
						}
					}
				}
			}
		}
		*/
		if (nID == m_nLoginCmd)
		{
			*state |= PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_nPBOMCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_nMaterialCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_nPBomEditCmdl && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_nCheckinPBomCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_CreatProcCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_ProcQueryCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_ProcCheckInCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_ProcPdfEditCmd && g_islogin)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_ImportEquiesCmd && g_islogin /*&& isshowImportEquies*/)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else if (nID == m_UpdatePartCmd && g_islogin &&isshowUpdatePart)
		{
			*state = PLUGIN_CMD_UI_ENABLE;
		}
		else
		{
			*state &= ~PLUGIN_CMD_UI_ENABLE;
		}
	}

	return S_OK;
}

STDMETHODIMP CwindchillObject::OnMainFrameCreated()
{
	IAPSRibbonBarPtr pRibbonBar = m_pApplication->GetRibbonBar();
	IAPSRibbonFactoryPtr pFactory = pRibbonBar->GetRibbonFactory();
	IAPSRibbonTabPtr pTab = pFactory->CreateRibbonTab();

	IAPSRibbonGroupPtr pPanel1 = pFactory->CreateRibbonGroup();
	IAPSRibbonButtonPtr pBtn10 = pFactory->CreateRibbonButton();

	pBtn10->put_Label(_bstr_t("��¼Windchill"));
	pBtn10->put_Enable(true);
	m_nLoginCmd = pBtn10->GetCmdID();		//��¼����ID
	pPanel1->AddChild(pBtn10);
	//pPanel1->put_Visible(VARIANT_FALSE);
	pPanel1->put_Label(_bstr_t("��¼"));

	IAPSRibbonGroupPtr pPanel2 = pFactory->CreateRibbonGroup();
	IAPSRibbonButtonPtr pBtn20 = pFactory->CreateRibbonButton();

	pBtn20->put_Label(_bstr_t("����PBOM"));
	(IAPSRibbonElementPtr)pBtn20->put_Enable(false);
	m_nPBOMCmd = pBtn20->GetCmdID();		//��¼����ID
	pPanel2->AddChild(pBtn20);
	IAPSRibbonButtonPtr pBtn21 = pFactory->CreateRibbonButton();
	pBtn21->put_Label(_bstr_t("PBOM�༭"));
	(IAPSRibbonElementPtr)pBtn21->put_Enable(false);
	m_nPBomEditCmdl = pBtn21->GetCmdID();		//��¼����ID
	pPanel2->AddChild(pBtn21);

	IAPSRibbonButtonPtr pBtn22 = pFactory->CreateRibbonButton();
	pBtn22->put_Label(_bstr_t("PBOM����"));
	(IAPSRibbonElementPtr)pBtn22->put_Enable(false);
	m_nCheckinPBomCmd = pBtn22->GetCmdID();		//��¼����ID
	pPanel2->AddChild(pBtn22);
	pPanel2->put_Label(_bstr_t("PBOM"));

	IAPSRibbonGroupPtr pPanel3 = pFactory->CreateRibbonGroup();
	IAPSRibbonButtonPtr pBtn30 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn30->put_Enable(false);
	pBtn30->put_Label(_bstr_t("���϶���༭"));
	m_nMaterialCmd = pBtn30->GetCmdID();		//��¼����ID
	pPanel3->AddChild(pBtn30);
	pPanel3->put_Label(_bstr_t("���϶���"));

	IAPSRibbonGroupPtr pPanel4 = pFactory->CreateRibbonGroup();
	IAPSRibbonButtonPtr pBtn40 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn40->put_Enable(false);
	pBtn40->put_Label(_bstr_t("װ�乤�մ���"));
	m_CreatProcCmd = pBtn40->GetCmdID();		//��¼����ID
	pPanel4->AddChild(pBtn40);

	IAPSRibbonButtonPtr pBtn41 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn41->put_Enable(false);
	pBtn41->put_Label(_bstr_t("װ�乤�ձ༭"));
	m_ProcQueryCmd = pBtn41->GetCmdID();		//��¼����ID
	pPanel4->AddChild(pBtn41);

	IAPSRibbonButtonPtr pBtn42 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn42->put_Enable(false);
	pBtn42->put_Label(_bstr_t("װ�乤�ռ���"));
	m_ProcCheckInCmd = pBtn42->GetCmdID();		//��¼����ID
	pPanel4->AddChild(pBtn42);
	pPanel4->put_Label(_bstr_t("װ�乤��"));


	IAPSRibbonGroupPtr pPanel5 = pFactory->CreateRibbonGroup();
	IAPSRibbonButtonPtr pBtn50 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn50->put_Enable(false);
	pBtn50->put_Label(_bstr_t("���չ��pdf�༭"));
	m_ProcPdfEditCmd = pBtn50->GetCmdID();		//��¼����ID
	pPanel5->AddChild(pBtn50);

	IAPSRibbonButtonPtr pBtn51 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn51->put_Enable(false);
	pBtn51->put_Label(_bstr_t("���빤��װ��"));
	m_ImportEquiesCmd = pBtn51->GetCmdID();		//��¼����ID
	pPanel5->AddChild(pBtn51);

	IAPSRibbonButtonPtr pBtn52 = pFactory->CreateRibbonButton();
	(IAPSRibbonElementPtr)pBtn52->put_Enable(false);
	pBtn52->put_Label(_bstr_t("�����㲿��"));
	m_UpdatePartCmd = pBtn52->GetCmdID();		//��¼����ID
	pPanel5->AddChild(pBtn52);

	pTab->put_Label(_bstr_t("Windchill����"));
	pTab->AddGroup(pPanel1);
	pTab->AddGroup(pPanel2);
	pTab->AddGroup(pPanel3);
	pTab->AddGroup(pPanel4);
	pTab->AddGroup(pPanel5);
	pRibbonBar->AddRibbonTab(pTab);
	//pRibbonBar->Layout(); ͳһ�ڴ�����ˢ��TAB

	m_PdfConfigDatas = new CArray<CAPSGridPropData*>;
	m_ConfigProcPDFName = new std::vector<std::map<CString, UINT>>;

	return S_OK;
}

STDMETHODIMP  CwindchillObject::raw_GetEnable(VARIANT_BOOL * pbEnable)
{
	return S_OK;
}

STDMETHODIMP CwindchillObject::raw_GetCookie(long * pdwCookie)
{
	return S_OK;
}
