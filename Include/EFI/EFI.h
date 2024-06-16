/*
 * Copyright (c) 2024 N11 Software. All rights reserved.
 */

#ifndef EFI_H
#define EFI_H

#define EFI_RUNTIME_SERVICE
#define EFI_API

#define EFI_MAX_BIT		0x800000000

#define EFIERR(a)		(EFI_MAX_BIT | (a))
#define EFIWARN(a)		(a)
#define EFI_ERROR(a)		(((INTN) (a)) < 0)

#define EFI_SUCCESS		0
#define EFI_LOAD_ERROR		EFIERR (1)
#define EFI_INVAL_PARAM		EFIERR (2)
#define EFI_UNSUPPORTED		EFIERR (3)
#define EFI_BAD_BUFFER_SIZE	EFIERR (4)

#endif /* EFI_H */
