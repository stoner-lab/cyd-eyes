#include "eye_presets.h"

// 320x240 landscape, eye centre around y = 120.
//
// Two ideas carry the "tired" family and they must not be confused:
//   openness  squashes the whole eye, which reads as squinting
//   lidDrop   keeps the eye size and drops the upper lid over it, which is what
//             an actual heavy eyelid looks like
// closedArc > 1 replaces the body with a curve, for eyes that are shut.
//
// There is no mouth: on a 320x240 panel it had to overlap the eyes to look like
// a yawn, and the overlap read as a mistake rather than a gesture.
const EyePreset EYE_PRESETS[EM_COUNT] = {
    /* NORMAL    */ {1.00f, 48.0f, 56.0f, 16.0f, 0.0f, 68.0f, 0.0f, 12.0f, 46.0f, 4.0f, 0.00f, 0.0f, ""},
    /* HAPPY     */ {0.82f, 46.0f, 52.0f, 14.0f, -4.0f, 68.0f, -5.0f, 14.0f, 44.0f, 6.0f, 0.06f, 0.0f, ""},
    /* PLAYFUL   */ {1.00f, 50.0f, 54.0f, 18.0f, -2.0f, 70.0f, 4.0f, 16.0f, 46.0f, 6.0f, 0.00f, 0.0f, ""},
    /* HUNGRY    */ {1.00f, 52.0f, 60.0f, 12.0f, 2.0f, 66.0f, 6.0f, 18.0f, 44.0f, 3.0f, 0.00f, 0.0f, "?"},
    /* SLEEPY    */ {0.55f, 46.0f, 52.0f, 12.0f, 3.0f, 66.0f, 0.0f, 12.0f, 42.0f, 2.0f, 0.35f, 0.0f, ""},
    /* ASLEEP    */ {0.02f, 48.0f, 56.0f, 13.0f, 2.0f, 68.0f, 0.0f, 14.0f, 42.0f, 3.0f, 0.00f, 6.0f, "Zzz"},
    /* SAD       */ {0.88f, 44.0f, 52.0f, 15.0f, 6.0f, 64.0f, -14.0f, 10.0f, 44.0f, -4.0f, 0.18f, 0.0f, ""},
    /* ANGRY     */ {0.95f, 48.0f, 52.0f, 13.0f, 0.0f, 66.0f, 20.0f, 8.0f, 42.0f, -3.0f, 0.34f, 0.0f, "!"},
    /* SURPRISED */ {1.00f, 54.0f, 64.0f, 10.0f, -2.0f, 72.0f, 0.0f, 20.0f, 48.0f, 8.0f, 0.00f, 0.0f, "!"},
    /* DAZED     */ {0.95f, 48.0f, 56.0f, 14.0f, 2.0f, 68.0f, 0.0f, 10.0f, 44.0f, 1.0f, 0.30f, 0.0f, ""},
    /* CONTENT   */ {0.02f, 48.0f, 56.0f, 16.0f, 0.0f, 68.0f, -6.0f, 15.0f, 44.0f, 5.0f, 0.00f, 12.0f, ""},
};
