#ifndef VMS_F11_HOME_H
#define VMS_F11_HOME_H
#include <stddef.h>
#include <stdint.h>

/* MOUNT/SRC/CHKSM2.B32 writes the computed little-endian word sum
 * at the requested byte offset and returns whether the old sum matched. */
int port_f11_checksum2(uint8_t block[512], unsigned checksum_byte);

/* MOUNT/SRC/CHKHM2.B32: returns 1 for the valid home block, 0 for
 * invalid fields, or the caller's volume-label mismatch code. */
uint32_t port_f11_check_home2(uint8_t block[512], uint32_t expected_lbn,
                               const char *requested_label, size_t label_length,
                               uint32_t label_mismatch,
                               int *is_structure_level2);
/* MOUNT/SRC/CHKHM1.B32: the ODS-1 versions 257 and 258.
 * ERR_EXIT for bad structure returns the caller's error code. */
uint32_t port_f11_check_home1(uint8_t block[512],
                              const char *requested_label, size_t label_length,
                              uint32_t label_mismatch,
                              uint32_t file_structure_error);
#endif
