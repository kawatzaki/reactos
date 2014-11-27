/*
 * ReactOS Explorer
 *
 * Copyright 2006 - 2007 Thomas Weidenmueller <w3seek@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "precomp.h"

/*****************************************************************************
 ** ITaskBand ****************************************************************
 *****************************************************************************/

const GUID CLSID_ITaskBand = { 0x68284FAA, 0x6A48, 0x11D0, { 0x8C, 0x78, 0x00, 0xC0, 0x4F, 0xD9, 0x18, 0xB4 } };

class CTaskBand :
    public CComCoClass<CTaskBand>,
    public CComObjectRootEx<CComMultiThreadModelNoCS>,
    public IObjectWithSite,
    public ITaskBand,
    public IDeskBand,
    public IDeskBar,
    public IPersistStream,
    public IWinEventHandler,
    public IOleCommandTarget
{
    CComPtr<ITrayWindow> Tray;
    CComPtr<IUnknown> punkSite;

    HWND hWnd;
    DWORD dwBandID;

public:
    CTaskBand() :
        hWnd(NULL),
        dwBandID(0)
    {

    }

    virtual ~CTaskBand() { }

    virtual HRESULT STDMETHODCALLTYPE GetRebarBandID(
        OUT DWORD *pdwBandID)
    {
        if (dwBandID != (DWORD) -1)
        {
            if (pdwBandID != NULL)
                *pdwBandID = dwBandID;

            return S_OK;
        }

        return E_FAIL;
    }

    /*****************************************************************************/

    virtual HRESULT STDMETHODCALLTYPE GetWindow(
        OUT HWND *phwnd)
    {

        /* NOTE: We have to return the tray window here so that ITaskBarClient
                 knows the parent window of the Rebar control it creates when
                 calling ITaskBarClient::SetDeskBarSite()! However, once we
                 created a window we return the task switch window! */
        if (hWnd != NULL)
            *phwnd = hWnd;
        else
            *phwnd = Tray->GetHWND();

        TRACE("ITaskBand::GetWindow(0x%p->0x%p)\n", phwnd, *phwnd);

        if (*phwnd != NULL)
            return S_OK;

        return E_FAIL;
    }

    virtual HRESULT STDMETHODCALLTYPE ContextSensitiveHelp(
        IN BOOL fEnterMode)
    {
        /* FIXME: Implement */
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE ShowDW(
        IN BOOL bShow)
    {
        /* We don't do anything... */
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE CloseDW(
        IN DWORD dwReserved)
    {
        /* We don't do anything... */
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE ResizeBorderDW(
        LPCRECT prcBorder,
        IUnknown *punkToolbarSite,
        BOOL fReserved) 
    {
        /* No need to implement this method */
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetBandInfo(
        IN DWORD dwBandID,
        IN DWORD dwViewMode,
        IN OUT DESKBANDINFO *pdbi)
    {
        TRACE("ITaskBand::GetBandInfo(0x%x,0x%x,0x%p) hWnd=0x%p\n", dwBandID, dwViewMode, pdbi, hWnd);

        if (hWnd != NULL)
        {
            /* The task band never has a title */
            pdbi->dwMask &= ~DBIM_TITLE;

            /* NOTE: We don't return DBIMF_UNDELETEABLE here, the band site will
                     handle us differently and add this flag for us. The reason for
                     this is future changes that might allow it to be deletable.
                     We want the band site to be in charge of this decision rather
                     the band itself! */
            /* FIXME: What about DBIMF_NOGRIPPER and DBIMF_ALWAYSGRIPPER */
            pdbi->dwModeFlags = DBIMF_VARIABLEHEIGHT;

            if (dwViewMode & DBIF_VIEWMODE_VERTICAL)
            {
                pdbi->ptIntegral.y = 1;
                pdbi->ptMinSize.y = 1;
                /* FIXME: Get the button metrics from the task bar object!!! */
                pdbi->ptMinSize.x = (3 * GetSystemMetrics(SM_CXEDGE) / 2) + /* FIXME: Might be wrong if only one column! */
                    GetSystemMetrics(SM_CXSIZE) + (2 * GetSystemMetrics(SM_CXEDGE)); /* FIXME: Min button size, query!!! */
            }
            else
            {
                pdbi->ptMinSize.y = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE)); /* FIXME: Query */
                pdbi->ptIntegral.y = pdbi->ptMinSize.y + (3 * GetSystemMetrics(SM_CYEDGE) / 2); /* FIXME: Query metrics */
                /* We're not going to allow task bands where not even the minimum button size fits into the band */
                pdbi->ptMinSize.x = pdbi->ptIntegral.y;
            }

            /* Ignored: pdbi->ptMaxSize.x */
            pdbi->ptMaxSize.y = -1;

            /* FIXME: We should query the height from the task bar object!!! */
            pdbi->ptActual.y = GetSystemMetrics(SM_CYSIZE) + (2 * GetSystemMetrics(SM_CYEDGE));

            /* Save the band ID for future use in case we need to check whether a given band
               is the task band */
            this->dwBandID = dwBandID;

            TRACE("H: %d, Min: %d,%d, Integral.y: %d Actual: %d,%d\n", (dwViewMode & DBIF_VIEWMODE_VERTICAL) == 0,
                pdbi->ptMinSize.x, pdbi->ptMinSize.y, pdbi->ptIntegral.y,
                pdbi->ptActual.x, pdbi->ptActual.y);

            return S_OK;
        }

        return E_FAIL;
    }

    /*****************************************************************************/
    // *** IOleCommandTarget methods ***
    virtual HRESULT STDMETHODCALLTYPE QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds [], OLECMDTEXT *pCmdText)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdexecopt, VARIANT *pvaIn, VARIANT *pvaOut)
    {
        if (IsEqualIID(*pguidCmdGroup, IID_IBandSite))
        {
            return S_OK;
        }

        if (IsEqualIID(*pguidCmdGroup, IID_IDeskBand))
        {
            return S_OK;
        }

        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    /*****************************************************************************/
    
    virtual HRESULT STDMETHODCALLTYPE SetClient(
        IN IUnknown *punkClient)
    {
        TRACE("IDeskBar::SetClient(0x%p)\n", punkClient);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE GetClient(
        OUT IUnknown **ppunkClient)
    {
        TRACE("IDeskBar::GetClient(0x%p)\n", ppunkClient);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE OnPosRectChangeDB(
        IN RECT *prc)
    {
        TRACE("IDeskBar::OnPosRectChangeDB(0x%p=(%d,%d,%d,%d))\n", prc, prc->left, prc->top, prc->right, prc->bottom);
        if (prc->bottom - prc->top == 0)
            return S_OK;

        return S_FALSE;
    }

    /*****************************************************************************/

    virtual HRESULT STDMETHODCALLTYPE GetClassID(
        OUT CLSID *pClassID)
    {
        TRACE("ITaskBand::GetClassID(0x%p)\n", pClassID);
        /* We're going to return the (internal!) CLSID of the task band interface */
        *pClassID = CLSID_ITaskBand;
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE IsDirty()
    {
        /* The object hasn't changed since the last save! */
        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE Load(
        IN IStream *pStm)
    {
        TRACE("ITaskBand::Load called\n");
        /* Nothing to do */
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE Save(
        IN IStream *pStm,
        IN BOOL fClearDirty)
    {
        /* Nothing to do */
        return S_OK;
    }

    virtual HRESULT STDMETHODCALLTYPE GetSizeMax(
        OUT ULARGE_INTEGER *pcbSize)
    {
        TRACE("ITaskBand::GetSizeMax called\n");
        /* We don't need any space for the task band */
        pcbSize->QuadPart = 0;
        return S_OK;
    }

    /*****************************************************************************/

    virtual HRESULT STDMETHODCALLTYPE SetSite(IUnknown *pUnkSite)
    {
        HRESULT hRet = E_FAIL;

        TRACE("ITaskBand::SetSite(0x%p)\n", pUnkSite);

        /* Release the current site */
        if (punkSite != NULL)
        {
            punkSite->Release();
        }

        punkSite = NULL;
        hWnd = NULL;

        if (pUnkSite != NULL)
        {
            IOleWindow *OleWindow;

            /* Check if the site supports IOleWindow */
            hRet = pUnkSite->QueryInterface(IID_PPV_ARG(IOleWindow, &OleWindow));
            if (SUCCEEDED(hRet))
            {
                HWND hWndParent = NULL;

                hRet = OleWindow->GetWindow(
                    &hWndParent);
                if (SUCCEEDED(hRet))
                {
                    /* Attempt to create the task switch window */

                    TRACE("CreateTaskSwitchWnd(Parent: 0x%p)\n", hWndParent);
                    hWnd = CreateTaskSwitchWnd(hWndParent, Tray);
                    if (hWnd != NULL)
                    {
                        punkSite = pUnkSite;
                        hRet = S_OK;
                    }
                    else
                    {
                        TRACE("CreateTaskSwitchWnd() failed!\n");
                        OleWindow->Release();
                        hRet = E_FAIL;
                    }
                }
                else
                    OleWindow->Release();
            }
            else
                TRACE("Querying IOleWindow failed: 0x%x\n", hRet);
        }

        return hRet;
    }

    virtual HRESULT STDMETHODCALLTYPE GetSite(
        IN REFIID riid,
        OUT VOID **ppvSite)
    {
        TRACE("ITaskBand::GetSite(0x%p,0x%p)\n", riid, ppvSite);

        if (punkSite != NULL)
        {
            return punkSite->QueryInterface(riid, ppvSite);
        }

        *ppvSite = NULL;
        return E_FAIL;
    }

    /*****************************************************************************/

    virtual HRESULT STDMETHODCALLTYPE ProcessMessage(
        IN HWND hWnd,
        IN UINT uMsg,
        IN WPARAM wParam,
        IN LPARAM lParam,
        OUT LRESULT *plrResult)
    {
        TRACE("ITaskBand: IWinEventHandler::ProcessMessage(0x%p, 0x%x, 0x%p, 0x%p, 0x%p)\n", hWnd, uMsg, wParam, lParam, plrResult);
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE ContainsWindow(
        IN HWND hWnd)
    {
        if (hWnd == hWnd ||
            IsChild(hWnd, hWnd))
        {
            TRACE("ITaskBand::ContainsWindow(0x%p) returns S_OK\n", hWnd);
            return S_OK;
        }

        return S_FALSE;
    }

    virtual HRESULT STDMETHODCALLTYPE OnWinEvent(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *theResult)
    {
        UNIMPLEMENTED;
        return E_NOTIMPL;
    }

    virtual HRESULT STDMETHODCALLTYPE IsWindowOwner(HWND hWnd)
    {
        return (hWnd == this->hWnd) ? S_OK : S_FALSE;
    }

    /*****************************************************************************/

    HRESULT STDMETHODCALLTYPE _Init(IN OUT ITrayWindow *tray)
    {
        Tray = tray;
        dwBandID = (DWORD) -1;
        return S_OK;
    }

    DECLARE_NOT_AGGREGATABLE(CTaskBand)

    DECLARE_PROTECT_FINAL_CONSTRUCT()
    BEGIN_COM_MAP(CTaskBand)
        COM_INTERFACE_ENTRY2_IID(IID_IOleWindow, IOleWindow, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IDeskBand, IDeskBand)
        COM_INTERFACE_ENTRY_IID(IID_IObjectWithSite, IObjectWithSite)
        COM_INTERFACE_ENTRY_IID(IID_IPersist, IPersist)
        COM_INTERFACE_ENTRY_IID(IID_IPersistStream, IPersistStream)
        COM_INTERFACE_ENTRY_IID(IID_IWinEventHandler, IWinEventHandler)
        COM_INTERFACE_ENTRY_IID(IID_IOleCommandTarget, IOleCommandTarget)
    END_COM_MAP()
};

ITaskBand * CreateTaskBand(IN OUT ITrayWindow *Tray)
{
    HRESULT hr;

    CTaskBand * tb = new CComObject<CTaskBand>();

    if (!tb)
        return NULL;

    hr = tb->AddRef();

    hr = tb->_Init(Tray);

    if (FAILED_UNEXPECTEDLY(hr))
        tb->Release();

    return tb;
}