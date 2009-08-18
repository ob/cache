#include <efi.h>
#include <efilib.h>

EFI_STATUS
efi_main(EFI_HANDLE image_handle, EFI_SYSTEM_TABLE *systab)
{
	EFI_TIME_CAPABILITIES	cap;
	EFI_TIME		time;
	EFI_STATUS		status;

	InitializeLib(image_handle, systab);
	Print(L"Hello World!\n");
	status = RT->GetTime(&time, &cap);
	if (status != EFI_SUCCESS) {
		Print(L"%r = GetTime()\n", status);
	} else {
		Print(L"%d/%d/%d - %d:%d:%d.%d\n",
		    time.Month, time.Day, time.Year,
		    time.Hour, time.Minute, time.Second, time.Nanosecond);
	}
	return EFI_SUCCESS;
}
