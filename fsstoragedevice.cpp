//
// fsstoragedevice.cpp
//
// This is sample that enumerates disk device for the local computer which 
// including inactive devices with the Windows Setup API.
// For example, it can found previously mounted USB memory too.
//
#include "stdafx.h"
#include "devproperty.h"

#define INITGUID
#include <devpkey.h>

class CDevicePropertySet
{
public:
    CDeviceProperty FriendlyName;
    CDeviceProperty DevNodeStatus;
    CDeviceProperty InstallDate;
    CDeviceProperty FirstInstallDate;
    CDeviceProperty RemovalRelations;
};

LONG _FindVolumeObjectPath(PCWSTR pszDeviceInstanceId,PWSTR pszVolume,DWORD cchVolume)
{
    LONG Status;

    Status = ERROR_PATH_NOT_FOUND;
    *pszVolume = L'\0';

    HDEVINFO hDevInfo;
    hDevInfo = SetupDiGetClassDevsEx(&GUID_DEVCLASS_VOLUME,NULL,NULL,DIGCF_PROFILE,NULL,NULL,NULL);

    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
        SP_DEVINFO_DATA DeviceInfoData = {0};
        DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

        if( SetupDiOpenDeviceInfo(hDevInfo,pszDeviceInstanceId,NULL,0,&DeviceInfoData) )
        {
            WCHAR szVolume[MAX_PATH];
            DEVPROPTYPE PropertyType;

            if( SetupDiGetDeviceProperty(hDevInfo,&DeviceInfoData,
                    &DEVPKEY_Device_PDOName,&PropertyType,
                    (PBYTE)szVolume,sizeof(szVolume),NULL,0) )
            {
                if( SUCCEEDED(StringCchCopy(pszVolume,cchVolume,szVolume)) )
                {
                    Status = ERROR_SUCCESS;
                }
            }
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }

    return Status;
}

BOOL _FindDosDriveLetter(PCWSTR pszVolume,PWSTR pszDosDrive,DWORD cchDosDrive)
{
    WCHAR szDrives[ (4 * 26) + 1 ]; // 'x'+':'+'\'+'\0' x='A'~'Z'
    GetLogicalDriveStrings(sizeof(szDrives)/sizeof(WCHAR),szDrives);

    WCHAR *pszDrive;
    pszDrive = szDrives;

    WCHAR szTargetPath[MAX_PATH];
    WCHAR szDrive[3];
    while( *pszDrive )
    {
        szDrive[0] = *pszDrive;
        szDrive[1] = L':';
        szDrive[2] = L'\0';

        szTargetPath[0] = L'\0';

        QueryDosDevice(szDrive,szTargetPath,MAX_PATH);

        if( _wcsicmp(szTargetPath,pszVolume) == 0 )
        {
            if( SUCCEEDED(StringCchCopy(pszDosDrive,cchDosDrive,szDrive)) )
            {
                SetLastError(ERROR_SUCCESS);
                return TRUE;
            }

            SetLastError(ERROR_INSUFFICIENT_BUFFER);
            return FALSE;
        }

        pszDrive += (wcslen(pszDrive) + 1);
    }

    *pszDosDrive = L'\0';

    SetLastError(ERROR_PATH_NOT_FOUND);

    return FALSE;
}

void _GetDateTimeString(const FILETIME *pft,LPTSTR pszText,int cchTextMax)
{
    SYSTEMTIME st;
    FILETIME ftLocal;

    FileTimeToLocalFileTime(pft,&ftLocal);
    FileTimeToSystemTime(&ftLocal,&st);

    int cch;
    cch = GetDateFormat(LOCALE_USER_DEFAULT,
                0,
                &st, 
                NULL,
                pszText,cchTextMax);

    pszText[cch-1] = L' ';

    GetTimeFormat(LOCALE_USER_DEFAULT,
                TIME_FORCE24HOURFORMAT,
                &st, 
                NULL,
                &pszText[cch],cchTextMax-cch);
}

void PrintFriendlyName(CDevicePropertySet &propSet)
{
    // Indicated an item with character of '*' is active device.
    //
    printf( "%c %S\n",
        (propSet.DevNodeStatus.IsValidData() && propSet.DevNodeStatus.GetUint32()) != 0 ? L'*' : L' ',
        propSet.FriendlyName.GetString());
 }

void PrintDateTime(CDeviceProperty &prop)
{
    FILETIME ft;
    WCHAR sz[MAX_PATH];

    if( prop.GetFileTime(&ft) != NULL )
    {
        _GetDateTimeString(prop.GetFileTimePtr(),sz,ARRAYSIZE(sz));
        printf("    Install Date Time  : %S\n",sz);
    }
}

void PrintRelationVolumes(CDeviceProperty &prop)
{
    if( prop.GetPropertyType() == (DEVPROP_TYPEMOD_LIST|DEVPROP_TYPE_STRING) )
    {
        printf("    Volume Object Path :");

        PCWSTR p = prop.GetString();
        
        while( *p )
        {
            WCHAR szVolume[MAX_PATH];
            WCHAR szDrive[MAX_PATH];

            if( _FindVolumeObjectPath(p,szVolume,ARRAYSIZE(szVolume)) == 0 )
            {
                _FindDosDriveLetter(szVolume,szDrive,ARRAYSIZE(szDrive));

                if( szDrive[0] != L'\0' )
                    printf("%*c%S (%S)\n",(p == prop.GetString())?0:25,' ',szVolume,szDrive);
                else
                    printf("%*c%S\n",(p == prop.GetString())?0:25,' ',szVolume);
            }

            p += (wcslen(p) + 1);
        }
    }
}

int __cdecl _compare(const void *a, const void *b)   
{
    CDevicePropertySet *set1 = *(CDevicePropertySet **)a;
    CDevicePropertySet *set2 = *(CDevicePropertySet **)b;

    return _wcsicmp(set1->FriendlyName.GetString(),set2->FriendlyName.GetString());
}

int wmain(int /*argc*/, WCHAR* /*argv[]*/)
{
    _wsetlocale(LC_ALL, L"");

    CSimpleArray<CDevicePropertySet *> devProps; 

    HDEVINFO hDevInfo;
    hDevInfo = SetupDiGetClassDevsEx(&GUID_DEVCLASS_DISKDRIVE,NULL,NULL,DIGCF_PROFILE,NULL,NULL,NULL);

    if( hDevInfo != INVALID_HANDLE_VALUE )
    {
        SP_DEVINFO_DATA DevInfoData = {0};
        DevInfoData.cbSize = sizeof(DevInfoData);

        DWORD dwIndex = 0;
        while( SetupDiEnumDeviceInfo(hDevInfo,dwIndex,&DevInfoData) )
        {
            CDevicePropertySet *set = new CDevicePropertySet;

            if( set != NULL )
            {
                if( !set->FriendlyName.GetProperty(hDevInfo, DevInfoData, DEVPKEY_Device_FriendlyName) )
                {
                    // When could not acquire friendly name, instead get the Instance ID as a friendly name.
                    // Possible cause is the target device deleted after call SetupDiEnumDeviceInfo().
                    //
                    set->FriendlyName.GetProperty(hDevInfo, DevInfoData, DEVPKEY_Device_InstanceId);
                }

                if( set->FriendlyName.IsValidData() )
                {
                    set->FriendlyName.GetProperty(hDevInfo, DevInfoData, DEVPKEY_Device_FriendlyName);
                    set->DevNodeStatus.GetProperty(hDevInfo,DevInfoData,DEVPKEY_Device_DevNodeStatus);
                    set->FriendlyName.GetProperty(hDevInfo,DevInfoData,DEVPKEY_Device_FriendlyName);
                    set->InstallDate.GetProperty(hDevInfo,DevInfoData,DEVPKEY_Device_InstallDate);
                    set->FirstInstallDate.GetProperty(hDevInfo,DevInfoData,DEVPKEY_Device_FirstInstallDate);
                    set->RemovalRelations.GetProperty(hDevInfo,DevInfoData,DEVPKEY_Device_RemovalRelations);

                    devProps.Add( set );
                }
                else
                {
                    // When could not make friendly name, free an object.
                    // But it normally does not occurs.
                    //
                    delete set;
                }
            }

            dwIndex++;
        }

        SetupDiDestroyDeviceInfoList(hDevInfo);
    }
    else
    {
        return -1; // error
    }

    // sort by friendly name
    //
    qsort(devProps.GetData(),devProps.GetSize(),sizeof(CDevicePropertySet *),_compare);

    // print information
    //
    int i,c;
    c = devProps.GetSize();
    for(i = 0; i < c; i++)
    {
        PrintFriendlyName(*devProps[i]);

        PrintDateTime(devProps[i]->InstallDate);

        PrintRelationVolumes(devProps[i]->RemovalRelations);

        printf("\n");
    }

    // frees object memory
    //
    c = devProps.GetSize();
    for(i = 0; i < c; i++)
    {
        delete devProps[i];
    }

    // frees pointer array
    //
    devProps.RemoveAll();

    return 0;
}
