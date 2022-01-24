// Console.c - Handles console functions
#include "console.h"
#include "string.h"
#include "types.h"
#include "vga.h"
#include "keyboard.h"

uint16 *g_vga_buffer;
uint32 g_vga_index;
uint8 cursorx = 0, cursory = 0;
uint8 g_fore_color = COLOR_WHITE, g_back_color = COLOR_BLACK;




void consolePrintColorString(char *str, VGA_COLOR_TYPE fore_color, VGA_COLOR_TYPE back_color) {
    uint8 tmp1 = g_fore_color;
    uint8 tmp2 = g_back_color;
    g_fore_color = fore_color;
    g_back_color = back_color;
    consolePrintString(str);
    g_fore_color = tmp1;
    g_back_color = tmp2;
    
}

uint16 get_box_draw_char(uint8 chn, uint8 fore_color, uint8 back_color)
{
  uint16 ax = 0;
  uint8 ah = 0;

  ah = back_color;
  ah <<= 4;
  ah |= fore_color;
  ax = ah;
  ax <<= 8;
  ax |= chn;

  return ax;
}


void draw_generic_box(uint16 x, uint16 y, 
                      uint16 width, uint16 height,
                      uint8 fore_color, uint8 back_color,
                      uint8 topleft_ch,
                      uint8 topbottom_ch,
                      uint8 topright_ch,
                      uint8 leftrightside_ch,
                      uint8 bottomleft_ch,
                      uint8 bottomright_ch)
{
  uint32 i;

  //increase vga_index to x & y location
  g_vga_index = 80*y;
  g_vga_index += x;

  //draw top-left box character
  g_vga_buffer[g_vga_index] = get_box_draw_char(topleft_ch, fore_color, back_color);

  g_vga_index++;
  //draw box top characters, -
  for(i = 0; i < width; i++){
    g_vga_buffer[g_vga_index] = get_box_draw_char(topbottom_ch, fore_color, back_color);
    g_vga_index++;
  }

  //draw top-right box character
  g_vga_buffer[g_vga_index] = get_box_draw_char(topright_ch, fore_color, back_color);

  // increase y, for drawing next line
  y++;
  // goto next line
  g_vga_index = 80*y;
  g_vga_index += x;

  //draw left and right sides of box
  for(i = 0; i < height; i++){
    //draw left side character
    g_vga_buffer[g_vga_index] = get_box_draw_char(leftrightside_ch, fore_color, back_color);
    g_vga_index++;
    //increase g_vga_index to the width of box
    g_vga_index += width;
    //draw right side character
    g_vga_buffer[g_vga_index] = get_box_draw_char(leftrightside_ch, fore_color, back_color);
    //goto next line
    y++;
    g_vga_index = 80*y;
    g_vga_index += x;
  }
  //draw bottom-left box character
  g_vga_buffer[g_vga_index] = get_box_draw_char(bottomleft_ch, fore_color, back_color);
  g_vga_index++;
  //draw box bottom characters, -
  for(i = 0; i < width; i++){
    g_vga_buffer[g_vga_index] = get_box_draw_char(topbottom_ch, fore_color, back_color);
    g_vga_index++;
  }
  //draw bottom-right box character
  g_vga_buffer[g_vga_index] = get_box_draw_char(bottomright_ch, fore_color, back_color);

  g_vga_index = 0;
}

void draw_box(uint8 boxtype, 
              uint16 x, uint16 y, 
              uint16 width, uint16 height,
              uint8 fore_color, uint8 back_color)
{
  switch(boxtype){
    case BOX_SINGLELINE : 
      draw_generic_box(x, y, width, height, 
                      fore_color, back_color, 
                      218, 196, 191, 179, 192, 217);
      break;

    case BOX_DOUBLELINE : 
      draw_generic_box(x, y, width, height, 
                      fore_color, back_color, 
                      201, 205, 187, 186, 200, 188);
      break;
  }
}

void fill_box(uint8 ch, uint16 x, uint16 y, uint16 width, uint16 height, uint8 color)
{
  uint32 i,j;

  for(i = 0; i < height; i++){
    //increase g_vga_index to x & y location
    g_vga_index = 80*y;
    g_vga_index += x;

    for(j = 0; j < width; j++){
      g_vga_buffer[g_vga_index] = get_box_draw_char(ch, 0, color);
      g_vga_index++;
    }
    y++;
  }
}



void clearConsole(VGA_COLOR_TYPE color1, VGA_COLOR_TYPE color2) {
    
    for (uint32 i = 0; i < VGA_TOTAL_ITEMS; i++) {
        g_vga_buffer[i] = vga_item_entry(NULL, color1, color2);
    }

    g_vga_index = 0;
    cursorx = 0, cursory = 0;
    vga_set_cursor_pos(cursorx, cursory);
}

void setColor(VGA_COLOR_TYPE fore_color, VGA_COLOR_TYPE back_color) {
    for (uint32 i = 0; i < VGA_TOTAL_ITEMS; i++) {
        g_vga_buffer[i] = vga_item_entry(g_vga_buffer[i], fore_color, back_color);
    }
    g_fore_color = fore_color;

}

void initConsole(VGA_COLOR_TYPE fore_color, VGA_COLOR_TYPE back_color) {
    g_vga_buffer = (uint16 *)VGA_ADDRESS;
    g_fore_color = fore_color, g_back_color = back_color;
    cursorx = 0, cursory = 0;
    clearConsole(fore_color, back_color);

}

static void consoleNewline() {
    if (cursory >= VGA_HEIGHT) {
        cursorx = 0, cursory = 0;
        clearConsole(g_fore_color, g_back_color);
    } else {
        g_vga_index += VGA_WIDTH - (g_vga_index % VGA_WIDTH);
        cursorx = 0;
        ++cursory;
        vga_set_cursor_pos(cursorx, cursory);
    }
}

void consolePutchar(char ch) {
    // Newline handling
    if (ch == '\n') {
        consoleNewline();
    } else if (ch == '\t')  { // Tab handling 
        for (int i=0; i<4;i++) {
            g_vga_buffer[g_vga_index++] = vga_item_entry(' ', g_fore_color, g_back_color);
            vga_set_cursor_pos(cursorx++, cursory);
        }
    } else if (ch == ' ') { // Space handling
        g_vga_buffer[g_vga_index++] = vga_item_entry(' ', g_fore_color, g_back_color);
        vga_set_cursor_pos(cursorx++, cursory);
    } else {
        if (ch > 0) {
            g_vga_buffer[g_vga_index++] = vga_item_entry(ch, g_fore_color, g_back_color);
            vga_set_cursor_pos(++cursorx, cursory);
        }
    }
}

void consoleUngetchar() {
    if(g_vga_index > 0) {
        g_vga_buffer[g_vga_index--] = vga_item_entry(0, g_fore_color, g_back_color);
        if(cursorx > 0) {
            vga_set_cursor_pos(cursorx--, cursory);
        } else {
            cursorx = VGA_WIDTH;
            if (cursory > 0)
                vga_set_cursor_pos(cursorx--, --cursory);
            else
                cursory = 0;
        }
    }

    g_vga_buffer[g_vga_index] = vga_item_entry(0, g_fore_color, g_back_color);
}

void consoleUngetcharBound(uint8 n) {
    if(((g_vga_index % VGA_WIDTH) > n) && (n > 0)) {
        g_vga_buffer[g_vga_index--] = vga_item_entry(0, g_fore_color, g_back_color);
        if(cursorx >= n) {
            vga_set_cursor_pos(cursorx--, cursory);
        } else {
            cursorx = VGA_WIDTH;
            if (cursory > 0)
                vga_set_cursor_pos(cursorx--, --cursory);
            else
                cursory = 0;
        }
    }

    g_vga_buffer[g_vga_index] = vga_item_entry(0, g_fore_color, g_back_color);
}

void consoleGoXY(uint16 x, uint16 y) {
    g_vga_index = (80*y)+x;
    cursorx = x, cursory = y;
    vga_set_cursor_pos(cursorx, cursory);
}

void consolePrintString(const char *str) {
    uint32 index = 0;
    while (str[index]) {
        if (str[index] == '\n')
            consoleNewline();
        else
            consolePutchar(str[index]);
        index++;
    }
}

void printf(const char *format, ...) {
    char **arg = (char **)&format;
    int c;
    char buf[32];

    arg++;

    memset(buf, 0, sizeof(buf));
    while ((c = *format++) != 0) {
        if (c != '%')
            consolePutchar(c);
        else {
            char *p, *p2;
            int pad0 = 0, pad = 0;

            c = *format++;
            if (c == '0') {
                pad0 = 1;
                c = *format++;
            }

            if (c >= '0' && c <= '9') {
                pad = c - '0';
                c = *format++;
            }

            switch (c) {
                case 'd':
                case 'u':
                case 'x':
                    itoa(buf, c, *((int *)arg++));
                    p = buf;
                    goto string;
                    break;

                case 's':
                    p = *arg++;
                    if (!p)
                        p = "(null)";

                string:
                    for (p2 = p; *p2; p2++)
                        ;
                    for (; p2 < p + pad; p2++)
                        consolePutchar(pad0 ? '0' : ' ');
                    while (*p)
                        consolePutchar(*p++);
                    break;

                default:
                    consolePutchar(*((int *)arg++));
                    break;
            }
        }
    }
}

void getString(char *buffer) {
    if (!buffer) return; // If not a buffer, exit.
    while (1) {
        char ch = kb_getchar();
        if (ch == '\n') {
            printf("\n");;
            return ;
        } else {
            *buffer++ = ch;
            printf("%c", ch);
        }
    }
}

void getStringBound(char *buffer, uint8 bound) {
    /* Basically same code as last time, but with some extra steps */
    if (!bound) return;
    while(1) {
        char ch = kb_getchar();
        if (ch == '\n') {
            printf("\n");
            return;
        } else if (ch == '\b') {
            consoleUngetcharBound(bound);
            buffer--;
            *buffer = '\0';
        } else {
            *buffer++ = ch;
            printf("%c", ch);
        }
    }
}