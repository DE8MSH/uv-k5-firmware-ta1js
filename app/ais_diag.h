/* Standalone on-device AIS RF activity diagnostic; NO GMSK bits.
 * Experimental AIS branch: Dr. Heinz Doofenshmirtz.
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef APP_AIS_DIAG_H
#define APP_AIS_DIAG_H
#include "ais_diag_core.h"
extern AIS_Diag gAisDiagnostic;
void AIS_DiagPoll10ms(void);
#endif
