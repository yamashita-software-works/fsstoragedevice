#pragma once

class CDeviceProperty
{
    PBYTE m_pBuffer;
    DWORD m_cbBuffer;
    DEVPROPTYPE m_PropertyType;
    DEVPROPKEY m_DevPropKey;

public:
    CDeviceProperty()
    {
        m_cbBuffer = 0;
        m_pBuffer = NULL;
        m_PropertyType = DEVPROP_TYPE_EMPTY;
        ZeroMemory(&m_DevPropKey,sizeof(DEVPROPKEY));
    }

    CDeviceProperty(const CDeviceProperty& prop)
    {
        m_pBuffer = (PBYTE)malloc( prop.GetBufferLength() );
        if( m_pBuffer )
        {
            m_cbBuffer = prop.GetBufferLength();
            memcpy(m_pBuffer,prop.GetBuffer(),m_cbBuffer);
            m_PropertyType = prop.GetPropertyType();
            m_DevPropKey = prop.GetPropKey();
        }
        else
        {
            m_cbBuffer = 0;
            m_PropertyType = 0;
            ZeroMemory(&m_DevPropKey,sizeof(DEVPROPKEY));
        }
    }

    ~CDeviceProperty()
    {
        if( m_pBuffer != NULL )
        {
            delete[] m_pBuffer;
            m_pBuffer = NULL;
        }
    }

    BOOL IsValidData() const
    {
        return (m_pBuffer != NULL);
    }

    PBYTE GetBuffer() const
    {
        return m_pBuffer;
    }

    DWORD GetBufferLength() const
    {
        return m_cbBuffer;
    }

    DEVPROPTYPE GetPropertyType() const
    {
        return m_PropertyType;
    }

    const DEVPROPKEY& GetPropKey() const
    {
        return m_DevPropKey;
    }

    PCWSTR GetString() const
    {
        return (PCWSTR)m_pBuffer;
    }

    const FILETIME *GetFileTimePtr() const
    {
        return (FILETIME *)m_pBuffer;
    }

    UINT32 GetUint32() const
    {
        if( m_pBuffer == NULL )
            return 0;
        return *(UINT32*)m_pBuffer;
    }

    FILETIME *GetFileTime(FILETIME* pft) const
    {
        if( m_pBuffer == NULL )
            return NULL;
        *pft = *(FILETIME *)m_pBuffer;
        return pft;
    }

    //
    // Get device property data
    //
    BOOL GetProperty(HDEVINFO hDevInfo,SP_DEVINFO_DATA& DevInfoData,const DEVPROPKEY& DevPropKey)
    {
        if( m_pBuffer != NULL )
        {
            // already data has
            return TRUE;
        }

        DEVPROPTYPE PropertyType;
        DWORD dwErrorCode;
        DWORD cb;

        do
        {
            SetupDiGetDeviceProperty(hDevInfo,&DevInfoData,
                    &DevPropKey,&PropertyType,
                    NULL,0,&cb,0);

            if( GetLastError() != ERROR_INSUFFICIENT_BUFFER  )
            {
                return FALSE; // invalid call or no data
            }

            m_pBuffer = new BYTE[ cb ];

            if( m_pBuffer == NULL )
            {
                return FALSE;
            }

            if( SetupDiGetDeviceProperty(hDevInfo,&DevInfoData,
                            &DevPropKey,&PropertyType,
                            m_pBuffer,cb,&cb,0) )
            {
                dwErrorCode = ERROR_SUCCESS;
                m_cbBuffer = cb;
                m_PropertyType = PropertyType;
                m_DevPropKey = DevPropKey;
            }
            else
            {
                dwErrorCode = GetLastError();
                delete[] m_pBuffer;
                m_pBuffer = NULL;
            }
        }
        while( dwErrorCode == ERROR_INSUFFICIENT_BUFFER );

        return IsValidData();
    }
};
