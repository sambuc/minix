#ifndef AK4531_H
#define AK4531_H
/* best viewed with tabsize=4 */

#include <minix/drivers.h>
#include <minix/sound.h>

int ak4531_init(uint16_t base, uint16_t status_reg, uint16_t bit, uint16_t poll);
int ak4531_get_set_volume(struct volume_level *level, int flag);

#endif
