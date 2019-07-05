#include <screen.h>

typedef struct
{
    unsigned int magic;
    unsigned int version;
    unsigned int headersize;
    unsigned int flags;
    unsigned int num_of_glyph;
    unsigned int bytes_per_glyph;
    unsigned int height;
    unsigned int width;
    unsigned char glyphs;
} __attribute__((packed)) psf_t;
extern volatile unsigned char _binary_font_psf_start;

int x, y;
unsigned int width, height, pitch;
// pitch is the nubmer of bytes in each row.
char *framebuffer;

#define DEPTH 32
#define SC_WIDTH 1024
#define SC_HEIGHT 768

int screen_init()
{
    x = 0;
    y = 0;                              // Initialize the position pointer of screen.
    mbox[0] = 35 * 4;                   // Length of message.
    mbox[1] = MBOX_REQUEST;             // Request message.

    mbox[2] = 0x48003;                  // Set physical width and height.
    mbox[3] = 8;
    mbox[4] = 8;
    mbox[5] = SC_WIDTH;                 // Physical width.
    mbox[6] = SC_HEIGHT;                // Physical height.

    mbox[7] = 0x48004;                  // Set virtual width and height.
    mbox[8] = 8;
    mbox[9] = 8;
    mbox[10] = SC_WIDTH;                // Virtual width, we set it same as physical width.
    mbox[11] = SC_HEIGHT;               // Virtual height, we set it same as physical height.

    mbox[12] = 0x48009;                 // Set virtual offset.
    mbox[13] = 8;                       
    mbox[14] = 8;
    mbox[15] = 0;                       // Set to 0.
    mbox[16] = 0;                       // Set to 0.

    mbox[17] = 0x48005;                 // Set depth of each pixel. 
    mbox[18] = 4;
    mbox[19] = 4;
    mbox[20] = DEPTH;                   // Set.

    mbox[21] = 0x48006;                 // Set pixel color order.
    mbox[22] = 4;
    mbox[23] = 4;
    mbox[24] = 1;                       // RGB.

    mbox[25] = 0x40001;                 // Get framebuffer. 
    mbox[26] = 8;
    mbox[27] = 8;
    mbox[28] = 4096;                    // FramebufferInfo pointer
    mbox[29] = 0;                       // FramebufferInfo size

    mbox[30] = 0x40008;                 // Get pitch.
    mbox[31] = 4;
    mbox[32] = 4;
    mbox[33] = 0;                       // FramebufferInfo pitch

    mbox[34] = MBOX_TAG_LAST;           // End of request.

    if(mbox_call(MBOX_CH_PROP) && mbox[20]==DEPTH && mbox[28]!=0)
    {
        mbox[28] &= 0x3FFFFFFF;
        width = mbox[5];
        height = mbox[6];
        pitch = mbox[33];
        framebuffer = (void *)((unsigned long)mbox[28]);
    }
    else
    {
        uart_puts("Unable to initialize screen.\n");
        return 0;
    }
    return 1;
}

void screen_print(const char *s)
{
    psf_t *font = (psf_t *)&_binary_font_psf_start;
    while (*s)
    {   
        // Get the start address of the character.
        unsigned char *glyph = (unsigned char *)&_binary_font_psf_start + font->headersize;
        if(*((unsigned char *)s) < font->num_of_glyph)
            glyph += (*s) * (font->bytes_per_glyph);
        else
            glyph += 0;

        // Offset of location.
        int offset = (y * font->height * pitch) + (x * (font->width + 1) * 4);
        // Bytes per line in the rom file.
        int bytesperline = (font->width + 7) / 8;
        int line_offset = 0;
        if(*s=='\r')
        {
            x = 0;
        }
        else if(*s=='\n')
        {
            x = 0;
            y += 1;
        }
        else
        {
            for (int i = 0; i < font->height; i++)      // Print a row.
            {
                line_offset = offset;
                for (int j = 0; j < font->width; j++)
                {
                    int mask = 1 << (font->width - 1 - j);
                    *((unsigned int *)(framebuffer + line_offset)) = ((int)*glyph) & mask ? 0xFFFFFF : 0;
                    line_offset += 4;
                }
                glyph += bytesperline;                  // Change the position of glyph.
                offset += pitch;                        // Go to next row.
            }
            x += 1;
        }
        s += 1;
    }
}