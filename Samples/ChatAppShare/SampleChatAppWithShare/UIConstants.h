#pragma once

#include <windows.h>

// Include resource.h for resource constants
#include "resource.h"

// Modern UI Color Scheme
#define COLOR_PRIMARY        RGB(64, 128, 255)      // Modern blue
#define COLOR_PRIMARY_DARK   RGB(45, 100, 220)     // Darker blue for hover
#define COLOR_APP_BACKGROUND RGB(248, 249, 250)    // Light gray background
#define COLOR_SURFACE        RGB(255, 255, 255)    // White surface
#define COLOR_TEXT_PRIMARY   RGB(33, 37, 41)       // Dark text
#define COLOR_TEXT_SECONDARY RGB(108, 117, 125)    // Gray text
#define COLOR_BORDER         RGB(222, 226, 230)    // Light border
#define COLOR_HOVER          RGB(248, 249, 250)    // Hover background
#define COLOR_CHAT_BUBBLE_OUT RGB(0, 123, 255)     // Outgoing message
#define COLOR_CHAT_BUBBLE_IN  RGB(233, 236, 239)   // Incoming message

// UI Constants
#define MAX_LOADSTRING 100
#define CONTACT_ITEM_HEIGHT 72
#define AVATAR_SIZE 40