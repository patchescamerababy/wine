/*
 * Copyright 2005 Jacek Caban
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "ole2.h"
#include "docobj.h"

#include "mshtml.h"
#include "mshtmhst.h"

#include "wine/debug.h"

#include "mshtml_private.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

/**********************************************************
 * IOleInPlaceActiveObject implementation
 */

#define ACTOBJ_THIS DEFINE_THIS(HTMLDocument, OleInPlaceActiveObject)

static HRESULT WINAPI OleInPlaceActiveObject_QueryInterface(IOleInPlaceActiveObject *iface, REFIID riid, void **ppvObject)
{
    ACTOBJ_THIS
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleInPlaceActiveObject_AddRef(IOleInPlaceActiveObject *iface)
{
    ACTOBJ_THIS
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleInPlaceActiveObject_Release(IOleInPlaceActiveObject *iface)
{
    ACTOBJ_THIS
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleInPlaceActiveObject_GetWindow(IOleInPlaceActiveObject *iface, HWND *phwnd)
{
    ACTOBJ_THIS
    TRACE("(%p)->(%p)\n", This, phwnd);

    if(!phwnd)
        return E_INVALIDARG;

    *phwnd = This->hwnd;
    return S_OK;
}

static HRESULT WINAPI OleInPlaceActiveObject_ContextSensitiveHelp(IOleInPlaceActiveObject *iface, BOOL fEnterMode)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%x)\n", This, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_TranslateAccelerator(IOleInPlaceActiveObject *iface, LPMSG lpmsg)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%p)\n", This, lpmsg);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_OnFrameWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_OnDocWindowActivate(IOleInPlaceActiveObject *iface, BOOL fActivate)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%x)\n", This, fActivate);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_ResizeBorder(IOleInPlaceActiveObject *iface, LPCRECT prcBorder,
                                                IOleInPlaceUIWindow *pUIWindow, BOOL fFrameWindow)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%p %p %x)\n", This, prcBorder, pUIWindow, fFrameWindow);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceActiveObject_EnableModeless(IOleInPlaceActiveObject *iface, BOOL fEnable)
{
    ACTOBJ_THIS
    FIXME("(%p)->(%x)\n", This, fEnable);
    return E_NOTIMPL;
}

static const IOleInPlaceActiveObjectVtbl OleInPlaceActiveObjectVtbl = {
    OleInPlaceActiveObject_QueryInterface,
    OleInPlaceActiveObject_AddRef,
    OleInPlaceActiveObject_Release,
    OleInPlaceActiveObject_GetWindow,
    OleInPlaceActiveObject_ContextSensitiveHelp,
    OleInPlaceActiveObject_TranslateAccelerator,
    OleInPlaceActiveObject_OnFrameWindowActivate,
    OleInPlaceActiveObject_OnDocWindowActivate,
    OleInPlaceActiveObject_ResizeBorder,
    OleInPlaceActiveObject_EnableModeless
};

#undef ACTOBJ_THIS

/**********************************************************
 * IOleInPlaceObjectWindowless implementation
 */

#define OLEINPLACEWND_THIS DEFINE_THIS(HTMLDocument, OleInPlaceObjectWindowless)

static HRESULT WINAPI OleInPlaceObjectWindowless_QueryInterface(IOleInPlaceObjectWindowless *iface,
        REFIID riid, void **ppvObject)
{
    OLEINPLACEWND_THIS
    return IHTMLDocument2_QueryInterface(HTMLDOC(This), riid, ppvObject);
}

static ULONG WINAPI OleInPlaceObjectWindowless_AddRef(IOleInPlaceObjectWindowless *iface)
{
    OLEINPLACEWND_THIS
    return IHTMLDocument2_AddRef(HTMLDOC(This));
}

static ULONG WINAPI OleInPlaceObjectWindowless_Release(IOleInPlaceObjectWindowless *iface)
{
    OLEINPLACEWND_THIS
    return IHTMLDocument2_Release(HTMLDOC(This));
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetWindow(IOleInPlaceObjectWindowless *iface,
        HWND *phwnd)
{
    OLEINPLACEWND_THIS
    return IOleWindow_GetWindow(OLEWIN(This), phwnd);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ContextSensitiveHelp(IOleInPlaceObjectWindowless *iface,
        BOOL fEnterMode)
{
    OLEINPLACEWND_THIS
    return IOleWindow_ContextSensitiveHelp(OLEWIN(This), fEnterMode);
}

static HRESULT WINAPI OleInPlaceObjectWindowless_InPlaceDeactivate(IOleInPlaceObjectWindowless *iface)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_UIDeactivate(IOleInPlaceObjectWindowless *iface)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_SetObjectRects(IOleInPlaceObjectWindowless *iface,
        LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)->(%p %p)\n", This, lprcPosRect, lprcClipRect);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_ReactivateAndUndo(IOleInPlaceObjectWindowless *iface)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_OnWindowMessage(IOleInPlaceObjectWindowless *iface,
        UINT msg, WPARAM wParam, LPARAM lParam, LRESULT *lpResult)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)->(%u %u %lu %p)\n", This, msg, wParam, lParam, lpResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI OleInPlaceObjectWindowless_GetDropTarget(IOleInPlaceObjectWindowless *iface,
        IDropTarget **ppDropTarget)
{
    OLEINPLACEWND_THIS
    FIXME("(%p)->(%p)\n", This, ppDropTarget);
    return E_NOTIMPL;
}

static IOleInPlaceObjectWindowlessVtbl OleInPlaceObjectWindowlessVtbl = {
    OleInPlaceObjectWindowless_QueryInterface,
    OleInPlaceObjectWindowless_AddRef,
    OleInPlaceObjectWindowless_Release,
    OleInPlaceObjectWindowless_GetWindow,
    OleInPlaceObjectWindowless_ContextSensitiveHelp,
    OleInPlaceObjectWindowless_InPlaceDeactivate,
    OleInPlaceObjectWindowless_UIDeactivate,
    OleInPlaceObjectWindowless_SetObjectRects,
    OleInPlaceObjectWindowless_ReactivateAndUndo,
    OleInPlaceObjectWindowless_OnWindowMessage,
    OleInPlaceObjectWindowless_GetDropTarget
};

#undef INPLACEWIN_THIS

void HTMLDocument_Window_Init(HTMLDocument *This)
{
    This->lpOleInPlaceActiveObjectVtbl = &OleInPlaceActiveObjectVtbl;
    This->lpOleInPlaceObjectWindowlessVtbl = &OleInPlaceObjectWindowlessVtbl;
}
