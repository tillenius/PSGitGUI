#include "visualstudio.h"
#include <Windows.h>
#include "atlbase.h"

std::wstring VisualStudioInterop::envdte = L"VisualStudio.DTE";

static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, const char * argument) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    CComBSTR bstr(argument);
    VARIANTARG variant_arg[1];
    VariantInit(&variant_arg[0]);
    variant_arg[0].vt = VT_BSTR;
    variant_arg[0].bstrVal = bstr;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

static bool InvokeMethod(CComPtr<IDispatch> & pDisp, const wchar_t * name, int arg1, int arg2) {
    DISPID dispid;
    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return false;

    CComVariant variant_result;
    EXCEPINFO exceptInfo;

    VARIANTARG variant_arg[2];
    VariantInit(&variant_arg[0]);
    VariantInit(&variant_arg[1]);
    variant_arg[0].vt = VT_INT;
    variant_arg[0].intVal = arg1;
    variant_arg[1].vt = VT_INT;
    variant_arg[1].intVal = arg2;
    DISPPARAMS args = { variant_arg, NULL, 1, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_METHOD, &args, &variant_result, &exceptInfo, NULL)))
        return false;
    return true;
}

static CComPtr<IDispatch> PropertyGetIDispatch(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return CComPtr<IDispatch>();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return CComPtr<IDispatch>();
    return variant_result.pdispVal;
}

static std::wstring PropertyGetBStr(CComPtr<IDispatch> & pDisp, const wchar_t * name) {
    EXCEPINFO exceptInfo;
    DISPID dispid;

    CComBSTR str(name);
    if (FAILED(pDisp->GetIDsOfNames(IID_NULL, &str, 1, LOCALE_USER_DEFAULT, &dispid)))
        return std::wstring();

    CComVariant variant_result;
    DISPPARAMS args = { NULL, NULL, 0, 0 };
    if (FAILED(pDisp->Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT, DISPATCH_PROPERTYGET, &args, &variant_result, &exceptInfo, NULL)))
        return std::wstring();
    return std::wstring(variant_result.bstrVal, SysStringLen(variant_result.bstrVal));
}

bool VisualStudioInterop::OpenInVS(const std::string & filename, int line) {
    CLSID clsid;
    if (FAILED(::CLSIDFromProgID(envdte.c_str(), &clsid)))
        return false;

    CComPtr<IUnknown> punk;
    if (FAILED(::GetActiveObject(clsid, NULL, &punk)))
        return false;

    CComPtr<IDispatch> pDisp;
    if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp))))
        return false;

    if (!InvokeMethod(pDisp, L"ExecuteCommand", (std::string("File.OpenFile ") + filename).c_str()))
        return false;

    CComPtr<IDispatch> activeDocument = PropertyGetIDispatch(pDisp, L"ActiveDocument");
    if (!activeDocument)
        return false;

    CComPtr<IDispatch> selection = PropertyGetIDispatch(activeDocument, L"Selection");
    if (!selection)
        return false;

    if (!InvokeMethod(pDisp, L"MoveToDisplayColumn", 1, line))
        return false;

    return true;
}

bool VisualStudioInterop::ExecuteCommand(const std::string & command) {
    CLSID clsid;
    if (FAILED(::CLSIDFromProgID(envdte.c_str(), &clsid)))
        return false;

    CComPtr<IUnknown> punk;
    if (FAILED(::GetActiveObject(clsid, NULL, &punk)))
        return false;

    CComPtr<IDispatch> pDisp;
    if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp))))
        return false;

    if (!InvokeMethod(pDisp, L"ExecuteCommand", command.c_str()))
        return false;

    return true;
}

std::wstring VisualStudioInterop::GetPathOfActiveDocument() {
    CLSID clsid;
    if (FAILED(::CLSIDFromProgID(envdte.c_str(), &clsid))) {
        return L"";
    }

    CComPtr<IUnknown> punk;
    if (FAILED(::GetActiveObject(clsid, NULL, &punk))) {
        return L"";
    }

    CComPtr<IDispatch> pDisp;
    if (FAILED(punk->QueryInterface(IID_PPV_ARGS(&pDisp)))) {
        return L"";
    }

    CComPtr<IDispatch> activeDocument = PropertyGetIDispatch(pDisp, L"ActiveDocument");
    if (!activeDocument) {
        return L"";
    }

    CComPtr<IDispatch> selection = PropertyGetIDispatch(activeDocument, L"Selection");
    if (!selection) {
        return L"";
    }

    std::wstring path = PropertyGetBStr(activeDocument, L"Path");
    return path;
}
