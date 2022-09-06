#include <types.h>

/**
 * Enum for function related to the user-frame list.
 * @enum {positionListq}
 */
enum positionList {
    addTOP,
    addBOTTOM,
    removeTOP,
    removeBOTTOM,
};

int addToFrameList(uint32_t value, uint8_t position);

int removeFromList(void);