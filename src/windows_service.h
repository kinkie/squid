/*
 * Copyright (C) 1996-2023 The Squid Software Foundation and contributors
 *
 * Squid software is distributed under GPLv2+ license and includes
 * contributions from numerous individuals and organizations.
 * Please see the COPYING and CONTRIBUTORS files for details.
 */

#ifndef SQUID_SRC_WINDOWS_SERVICE_H
#define SQUID_SRC_WINDOWS_SERVICE_H

#if _SQUID_WINDOWS_ || _SQUID_MINGW_
int WIN32_StartService(int, char **);
int WIN32_Subsystem_Init(int *, char ***);
void WIN32_sendSignal(int);
void WIN32_SetServiceCommandLine(void);
void WIN32_InstallService(void);
void WIN32_RemoveService(void);

DWORD WIN32_IpAddrChangeMonitorInit();
#else /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */
inline int WIN32_Subsystem_Init(int *, char ***) {return 0;} /* NOP */
inline void WIN32_sendSignal(int) {return;} /* NOP */
inline void WIN32_SetServiceCommandLine(void) {} /* NOP */
inline void WIN32_InstallService(void) {} /* NOP */
inline  void WIN32_RemoveService(void) {} /* NOP */
#endif /* _SQUID_WINDOWS_ || _SQUID_MINGW_ */

#endif /* SQUID_SRC_WINDOWS_SERVICE_H */

