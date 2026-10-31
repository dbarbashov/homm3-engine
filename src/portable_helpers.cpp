#include "kb.h"
#include "kbwin.h"
#include "soundmgr.h"
#include "palette.h"
#include "winmm_thunks.h"

// Original body extracted from kbwin.cpp
unsigned long GameTime::get()
{
    return timeGetTime();
}

// Original body extracted from kbwin.cpp
void GameTime::delayTil(unsigned long time)
{
    while (!GameTime::isPast(time)) {
        process1WindowsMessage();
        pollSound();
    }
}

// Original body extracted from kbwin.cpp
void GameTime::delay(int interval)
{
    GameTime::delayTil(GameTime::get() + interval);
}

// Original body extracted from kbwin.cpp
void initVideo()
{
}

// Original body extracted from wingraph.cpp
void setPlayerPaletteColors(unsigned short* pal, int whichPlayer)
{
    memcpy(pal + 224, &g_playerPalette->Palette[whichPlayer * 32],
           32 * sizeof(unsigned short));
}

// Original body extracted from wingraph.cpp
void setPlayerPaletteColors(TPalette24& pal, int whichPlayer)
{
    memcpy(pal.Palette + 224 * 3, g_playerPalette24->Palette + whichPlayer * 32 * 3,
           32 * 3);
}
#include "font.h"
#include <cstdarg>
#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))
static const int g_dialogIconMaxRows=2, g_dialogIconRowSingle=1, g_dialogIconRowPair=2, g_dialogIconRowTriple=3, g_dialogIconRowQuad=4;
void calculateNormalDialogSize(TNormalDialogInfo& dialogInfo)
{
    long widestIcon = 0;
    long spacing = 0;
    int labelHeight[g_dialogIconMaxRows];
    int iconHeight[g_dialogIconMaxRows];
    int iconRows;
    int i;
    int iconsPerLine;

    // DC 5219/5220 stores icon then label; chaining preserves that order.
    for (i = 0; i < g_dialogIconMaxRows; ++i) {
        labelHeight[i] = iconHeight[i] = 0;
    }

    int numIcons = 0;
    for (i = 0; i < 8; i++)
        if (dialogInfo.m_icons[i].m_resource != -1)
            numIcons++;
    iconRows = (numIcons + 3) / 4;
    if (iconRows > 0)
        iconsPerLine = (numIcons + iconRows - 1) / iconRows;
    for (i = 0; i < 8; i++) {
        if (dialogInfo.m_icons[i].m_resource != -1) {
            int row = i / iconsPerLine;
            widestIcon = max(dialogInfo.m_icons[i].m_spriteWidth, widestIcon);
            widestIcon = max(dialogInfo.m_icons[i].m_textWidth, widestIcon);
            spacing = max(spacing, dialogInfo.m_icons[i].m_textWidth
                                      - dialogInfo.m_icons[i].m_spriteWidth);
            iconHeight[row] = max(iconHeight[row],
                                        dialogInfo.m_icons[i].m_spriteHeight);
            labelHeight[row] = max(labelHeight[row],
                                      dialogInfo.m_icons[i].m_textHeight);
        }
    }

    dialogInfo.m_width = 40;
    if (numIcons > 0) {
        int perRow = (numIcons + iconRows - 1) / iconRows;
        // Mac retail reloads the initialized width for this maximum;
        // Dreamcast's max<long> call fixes the operand type.
        dialogInfo.m_width = max(static_cast<long>(dialogInfo.m_width),
                                 (perRow - 1) * (spacing + 40)
                                     + perRow * widestIcon + 40);
    }

    font* currentFont = g_mediumFont;
    dialogInfo.m_textExpansion = false;
    dialogInfo.m_textWidgetWidth = max(
        256, currentFont->longest_word_length(dialogInfo.m_dialogText.c_str()));
    int maximumWidth =
        currentFont->LongestLineWidth(dialogInfo.m_dialogText.c_str());
    // DC 5282 clears this byte; 5300 sets it after growth and 5308 tests it.
    unsigned char widthChanged;
    do {
        widthChanged = 0;
        int lines = currentFont->LineLength(dialogInfo.m_dialogText.c_str(),
                                    dialogInfo.m_textWidgetWidth);
        dialogInfo.m_textWidgetHeight = currentFont->fs.height * lines;
        if (maximumWidth <= dialogInfo.m_textWidgetWidth) {
            dialogInfo.m_textWidgetWidth = maximumWidth;
            break;
        }
        if (dialogInfo.m_textWidgetWidth < 578
            && (dialogInfo.m_textWidgetHeight > 200
                || 3 * dialogInfo.m_textWidgetHeight
                       > 2 * dialogInfo.m_textWidgetWidth)) {
            dialogInfo.m_textWidgetWidth =
                min(3 * dialogInfo.m_textWidgetWidth / 2, 578);
            widthChanged = 1;
        } else {
            dialogInfo.m_textWidgetWidth =
                currentFont->LongestWrappedLineWidth(
                    dialogInfo.m_dialogText.c_str(),
                    dialogInfo.m_textWidgetWidth);
        }
    } while (widthChanged);
    if (dialogInfo.m_textWidgetHeight > 200) {
        dialogInfo.m_textWidgetHeight = 200;
        dialogInfo.m_textExpansion = true;
    }

    dialogInfo.m_width = max(dialogInfo.m_width, dialogInfo.m_textWidgetWidth + 50);
    dialogInfo.m_width = (dialogInfo.m_width + 63) & ~63;
    if (dialogInfo.m_mbType == NORMAL_DIALOG_POPUP
        && dialogInfo.m_width < 128)
        dialogInfo.m_width = 128;
    else if (dialogInfo.m_mbType != NORMAL_DIALOG_POPUP
        && dialogInfo.m_width < 256)
        dialogInfo.m_width = 256;
    dialogInfo.m_textWidgetWidth = dialogInfo.m_width - 50;

    int boxHeight = 60;
    boxHeight += dialogInfo.m_textWidgetHeight;
    if (numIcons > 0) {
        boxHeight += 20;
        for (i = 0; i < iconRows; i++)
            boxHeight += labelHeight[i] + iconHeight[i];
        boxHeight += 20 * (iconRows - 1);
    }
    if (dialogInfo.m_mbType != NORMAL_DIALOG_POPUP)
        boxHeight += 50;
    dialogInfo.m_height = (boxHeight + 63) & ~63;
    dialogInfo.m_height = max(dialogInfo.m_height, 128);
    dialogInfo.m_textWidgetY += (dialogInfo.m_height - boxHeight) / 2;
    if (dialogInfo.m_x == -1 || dialogInfo.m_x + dialogInfo.m_width >= 799)
        dialogInfo.m_x = (800 - dialogInfo.m_width) / 2;
    if (dialogInfo.m_y == -1 || dialogInfo.m_y + dialogInfo.m_height >= 599)
        dialogInfo.m_y = (600 - dialogInfo.m_height) / 2;
    if (numIcons == 0)
        return;

    int iconY = dialogInfo.m_height - labelHeight[0] - 30;
    if (dialogInfo.m_mbType != NORMAL_DIALOG_POPUP)
        iconY -= 50;
    if (iconRows == g_dialogIconMaxRows)
        iconY -= labelHeight[1] + iconHeight[1] + 20;
    int firstInRow = 0;
    for (i = 0; i < iconRows; i++) {
        int rowsLeft = iconRows - i;
        int inRow = (numIcons - firstInRow + rowsLeft - 1) / rowsLeft;
        switch (inRow) {
        case g_dialogIconRowSingle:
            dialogInfo.m_icons[firstInRow].m_spriteX =
                (dialogInfo.m_width
                 - dialogInfo.m_icons[firstInRow].m_spriteWidth) / 2;
            break;
        case g_dialogIconRowPair:
            dialogInfo.m_icons[firstInRow].m_spriteX =
                (dialogInfo.m_width - spacing - 40) / 2
                - dialogInfo.m_icons[firstInRow].m_spriteWidth;
            dialogInfo.m_icons[firstInRow + 1].m_spriteX =
                (dialogInfo.m_width + spacing + 40) / 2;
            break;
        case g_dialogIconRowTriple:
            dialogInfo.m_icons[firstInRow + 1].m_spriteX =
                (dialogInfo.m_width
                 - dialogInfo.m_icons[firstInRow + 1].m_spriteWidth) / 2;
            dialogInfo.m_icons[firstInRow].m_spriteX =
                dialogInfo.m_icons[firstInRow + 1].m_spriteX
                - dialogInfo.m_icons[firstInRow].m_spriteWidth - 40;
            dialogInfo.m_icons[firstInRow + 2].m_spriteX =
                dialogInfo.m_icons[firstInRow + 1].m_spriteX
                + dialogInfo.m_icons[firstInRow + 1].m_spriteWidth + 40;
            break;
        case g_dialogIconRowQuad:
            dialogInfo.m_icons[firstInRow + 1].m_spriteX =
                (dialogInfo.m_width - spacing - 40) / 2
                - dialogInfo.m_icons[firstInRow + 1].m_spriteWidth;
            dialogInfo.m_icons[firstInRow].m_spriteX =
                dialogInfo.m_icons[firstInRow + 1].m_spriteX
                - dialogInfo.m_icons[firstInRow].m_spriteWidth - 40;
            dialogInfo.m_icons[firstInRow + 2].m_spriteX =
                (dialogInfo.m_width + spacing + 40) / 2;
            dialogInfo.m_icons[firstInRow + 3].m_spriteX =
                dialogInfo.m_icons[firstInRow + 2].m_spriteX
                + dialogInfo.m_icons[firstInRow + 2].m_spriteWidth + 40;
            break;
        }
        for (int k = 0; k < inRow; k++) {
            dialogInfo.m_icons[firstInRow].m_textY = iconY;
            dialogInfo.m_icons[firstInRow].m_spriteY =
                iconY - dialogInfo.m_icons[firstInRow].m_spriteHeight;
            dialogInfo.m_icons[firstInRow].m_textX =
                dialogInfo.m_icons[firstInRow].m_spriteX
                + (dialogInfo.m_icons[firstInRow].m_spriteWidth
                   - dialogInfo.m_icons[firstInRow].m_textWidth) / 2;
            // DC5430 advances the current icon before the inner backedge.
            ++firstInRow;
        }
        iconY += labelHeight[1] + iconHeight[1] + 20;
    }
}
std::string formatString(const char* format, ...)
{
    // Dreamcast CodeView types format_string's function static `buffer` as
    // char[512].
    DATA(0x006996cc)
    static char buffer[512];
    va_list arguments;
    va_start(arguments, format);
    vsprintf(buffer, format, arguments);
    va_end(arguments);
    return std::string(buffer);
}
