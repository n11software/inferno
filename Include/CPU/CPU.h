//========= Copyright N11 Software, All rights reserved. ============//
//
// File: CPU.h
// Purpose:
// Maintainer: atl
//
//===================================================================//
#ifndef CPU_H
#define CPU_H

namespace CPU {
	void CPUDetect();
        void IntelHandler();
        void AMDHandler();
        void UnknownHandler();
}

#endif //CPU_H
