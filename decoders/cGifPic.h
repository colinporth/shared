// cGifPic.h - remember the dodgy realloc
#pragma once
#include "iPic.h"
//{{{  defines
/*  Maximum colour table size */
#define GIF_MAX_COLOURS 256

/*  Internal flag that the colour table needs to be processed */
#define GIF_PROCESS_COLOURS 0xaa000000

/*  Internal flag that a frame is invalid/unprocessed */
#define GIF_INVALID_FRAME -1

/*  Maximum LZW bits available */
#define GIF_MAX_LZW 12

/* Transparent colour */
#define GIF_TRANSPARENT_COLOUR 0x00

/*  GIF Flags */
#define GIF_FRAME_COMBINE 1
#define GIF_FRAME_CLEAR 2
#define GIF_FRAME_RESTORE 3
#define GIF_FRAME_QUIRKS_RESTORE 4
#define GIF_IMAGE_SEPARATOR 0x2c
#define GIF_INTERLACE_MASK 0x40
#define GIF_COLOUR_TABLE_MASK 0x80
#define GIF_COLOUR_TABLE_SIZE_MASK 0x07
#define GIF_EXTENSION_INTRODUCER 0x21
#define GIF_EXTENSION_GRAPHIC_CONTROL 0xf9
#define GIF_DISPOSAL_MASK 0x1c
#define GIF_TRANSPARENCY_MASK 0x01
#define GIF_EXTENSION_COMMENT 0xfe
#define GIF_EXTENSION_PLAIN_TEXT 0x01
#define GIF_EXTENSION_APPLICATION 0xff
#define GIF_BLOCK_TERMINATOR 0x00
#define GIF_TRAILER 0x3b
//}}}

class cGifPic : public iPic {
public:
  cGifPic() {}
  //{{{
  ~cGifPic() {
    bigFree (mFrames);
    mFrames = NULL;

    bigFree (mlocal_colour_table);
    mlocal_colour_table = NULL;

    bigFree (mGlobal_colour_table);
    mGlobal_colour_table = NULL;
    }
  //}}}
  void* operator new (std::size_t size) { return smallMalloc (size, "cGifPic"); }
  void operator delete (void *ptr) { smallFree (ptr); }

  //{{{  enum eGifResult
  enum eGifResult {
    GIF_WORKING = 1,
    GIF_OK = 0,
    GIF_INSUFFICIENT_FRAME_DATA = -1,
    GIF_FRAME_DATA_ERROR = -2,
    GIF_INSUFFICIENT_DATA = -3,
    GIF_DATA_ERROR = -4,
    GIF_INSUFFICIENT_MEMORY = -5,
    GIF_FRAME_NO_DISPLAY = -6,
    GIF_END_OF_FRAME = -7
  };
  //}}}

  // iPic
  virtual uint16_t getWidth() { return mWidth; }
  virtual uint16_t getHeight() { return mHeight; }
  virtual uint16_t getComponents() { return 4; }
  virtual uint8_t* getPic() { return (uint8_t*)mPicBuffer; }
  //{{{
  virtual void setPic (const uint8_t* buffer, int size, uint16_t componenets) {
    mSrcBuffer = (uint8_t*)buffer;
    mFileSize = size;
    }
  //}}}

  //{{{
  eGifResult readHeader(){

    buffer_size = mFileSize;
    mGifData = mSrcBuffer;

    unsigned char* gifData;
    unsigned int index;
    eGifResult return_value;

    if (buffer_size < 13)
      return GIF_INSUFFICIENT_DATA;

    gifData = mGifData + buffer_position;

    if (buffer_position == 0) {
      /*  We want everything to be NULL before we start so we've no chance of freeing bad pointers (paranoia) */
      mPicBuffer = NULL;
      mFrames = NULL;
      mlocal_colour_table = NULL;
      mGlobal_colour_table = NULL;

      /*  The caller may have been lazy and not reset any values */
      mFrame_count = 0;
      mFrame_count_partial = 0;
      mdecoded_frame = GIF_INVALID_FRAME;

      /* 6-byte GIF file header is:
       *  +0  3CHARS  Signature ('GIF')
       *  +3  3CHARS  Version ('87a' or '89a') */
      if (strncmp((const char *) gifData, "GIF", 3) != 0)
        return GIF_DATA_ERROR;
      gifData += 3;
      // go past version
      gifData += 3;

      /* 7-byte Logical Screen Descriptor is:
       *  +0  SHORT Logical Screen Width
       *  +2  SHORT Logical Screen Height
       *  +4  CHAR  __Packed Fields__
       *      1BIT  Global Colour Table Flag
       *      3BITS Colour Resolution
       *      1BIT  Sort Flag
       *      3BITS Size of Global Colour Table
       *  +5  CHAR  Background Colour Index
       *  +6  CHAR  Pixel Aspect Ratio */
      mWidth = gifData[0] | (gifData[1] << 8);
      mHeight = gifData[2] | (gifData[3] << 8);
      mGlobal_colours = (gifData[4] & GIF_COLOUR_TABLE_MASK) != 0;
      mcolour_table_size = (2 << (gifData[4] & GIF_COLOUR_TABLE_SIZE_MASK));
      mbackground_index = gifData[5];
      maspect_ratio = gifData[6];
      mloop_count = 1;
      gifData += 7;

      /*  Some broken GIFs report the size as the screen size they were created in. As
          such, we detect for the common cases and set the sizes as 0 if they are found
          which results in the GIF being the maximum size of the frames. */
      if (((mWidth == 640) && (mHeight == 480)) ||
          ((mWidth == 640) && (mHeight == 512)) ||
          ((mWidth == 800) && (mHeight == 600)) ||
          ((mWidth == 1024) && (mHeight == 768)) ||
          ((mWidth == 1280) && (mHeight == 1024)) ||
          ((mWidth == 1600) && (mHeight == 1200)) ||
          ((mWidth == 0) || (mHeight == 0)) ||
          ((mWidth > 2048) || (mHeight > 2048))) {
        mWidth = 1;
        mHeight = 1;
        }

      /*  Allocate some data irrespective of whether we've got any colour tables. We
          always get the maximum size in case a GIF is lying to us. It's far better
          to give the wrong colours than to trample over some memory somewhere. */
      mGlobal_colour_table = (unsigned int*)bigMalloc(GIF_MAX_COLOURS * sizeof(unsigned int), "GIFglobal");
      mlocal_colour_table = (unsigned int*)bigMalloc(GIF_MAX_COLOURS * sizeof(unsigned int), "GIFlocal");
      if ((mGlobal_colour_table == NULL) || (mlocal_colour_table == NULL))
        return GIF_INSUFFICIENT_MEMORY;

      /*  Set the first colour to a value that will never occur in reality so we know if we've processed it */
      mGlobal_colour_table[0] = GIF_PROCESS_COLOURS;

      /*  Check if the GIF has no frame data (13-byte header + 1-byte termination block)
       *  Although generally useless, the GIF specification does not expressly prohibit this */
      if (buffer_size == 14) {
        if (gifData[0] == GIF_TRAILER)
          return GIF_OK;
        else
          return GIF_INSUFFICIENT_DATA;
        }

      if ((mFrames = (tGifFrame*)bigMalloc(sizeof(tGifFrame), "GIFframes")) == NULL)
        return GIF_INSUFFICIENT_MEMORY;
      mFrame_holders = 1;

      mPicBuffer = bigMalloc (mWidth*mHeight*4, "GIFimage");
      buffer_position = (unsigned int)(gifData - mGifData);
      }

    /*  Do the colour map if we haven't already. As the top byte is always 0xff or 0x00
        depending on the transparency we know if it's been filled in. */
    if (mGlobal_colour_table[0] == GIF_PROCESS_COLOURS) {
      /*  Check for a global colour map signified by bit 7 */
      if (mGlobal_colours) {
        if (buffer_size < (mcolour_table_size * 3 + 12))
          return GIF_INSUFFICIENT_DATA;
        for (index = 0; index < mcolour_table_size; index++) {
          /* Gif colour map contents are r,g,b.
           * We want to pack them bytewise into the colour table, such that the red component
           * is in byte 0 and the alpha component is in * byte 3. */
          unsigned char *entry = (unsigned char*)&mGlobal_colour_table[index];
          entry[0] = gifData[0]; /* r */
          entry[1] = gifData[1]; /* g */
          entry[2] = gifData[2]; /* b */
          entry[3] = 0xff;  /* a */
          gifData += 3;
          }
        buffer_position = (unsigned int)(gifData - mGifData);
        }
      else {
        /*  Create a default colour table with the first two colours as black and white */
        unsigned int* entry = mGlobal_colour_table;
        entry[0] = 0x00000000;
        /* Force Alpha channel to opaque */
        ((unsigned char *) entry)[3] = 0xff;
        entry[1] = 0xffffffff;
        }
      }

    /*  Repeatedly try to initialise frames */
    while ((return_value = gif_initialise_frame()) == GIF_WORKING);

    /*  If there was a memory error tell the caller */
    if ((return_value == GIF_INSUFFICIENT_MEMORY) || (return_value == GIF_DATA_ERROR))
      return return_value;

    /*  If we didn't have some frames then a GIF_INSUFFICIENT_DATA becomes a GIF_INSUFFICIENT_FRAME_DATA */
    if ((return_value == GIF_INSUFFICIENT_DATA) && (mFrame_count_partial > 0))
      return GIF_INSUFFICIENT_FRAME_DATA;

    /*  Return how many we got */
    return return_value;
    }
  //}}}
  //{{{
  eGifResult decodeBody (unsigned int frame) {

    unsigned int index = 0;
    unsigned char* gifData, *gif_end;
    int gif_bytes;
    unsigned int width, height, offset_x, offset_y;
    unsigned int flags, colour_table_size, interlace;
    unsigned int *colour_table;
    unsigned int *frame_data = 0; // Set to 0 for no warnings
    unsigned int *frame_scanline;
    unsigned int save_buffer_position;
    unsigned int return_value = 0;
    unsigned int x, y, decode_y, burst_bytes;
    int last_undisposed_frame = (frame - 1);
    unsigned char colour;

    /*  Ensure this frame is supposed to be decoded */
    if (mFrames[frame].display == false) {
      mcurrent_error = GIF_FRAME_NO_DISPLAY;
      return GIF_OK;
      }

    /*  Ensure we have a frame to decode */
    if (frame > mFrame_count_partial)
      return GIF_INSUFFICIENT_DATA;
    if ((!clear_image) && ((int)frame == mdecoded_frame))
      return GIF_OK;

    /*  Get the start of our frame data and the end of the GIF data */
    gifData = mGifData + mFrames[frame].frame_pointer;
    gif_end = mGifData + buffer_size;
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Check if we have enough data The shortest block of data is a 10-byte image descriptor + 1-byte gif trailer */
    if (gif_bytes < 12) return GIF_INSUFFICIENT_FRAME_DATA;

    /*  Save the buffer position */
    save_buffer_position = buffer_position;
    buffer_position = (unsigned int)(gifData - mGifData);

    /*  Skip any extensions because we all ready processed them */
    if ((return_value = gif_skip_frame_extensions()) != GIF_OK)
      goto gif_decode_frame_exit;
    gifData = (mGifData + buffer_position);
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Ensure we have enough data for the 10-byte image descriptor + 1-byte gif trailer */
    if (gif_bytes < 12) {
      return_value = GIF_INSUFFICIENT_FRAME_DATA;
      goto gif_decode_frame_exit;
      }

    /* 10-byte Image Descriptor is:
     *  +0  CHAR  Image Separator (0x2c)
     *  +1  SHORT Image Left Position
     *  +3  SHORT Image Top Position
     *  +5  SHORT Width
     *  +7  SHORT Height
     *  +9  CHAR  __Packed Fields__
     *      1BIT  Local Colour Table Flag
     *      1BIT  Interlace Flag
     *      1BIT  Sort Flag
     *      2BITS Reserved
     *      3BITS Size of Local Colour Table */
    if (gifData[0] != GIF_IMAGE_SEPARATOR) {
      return_value = GIF_DATA_ERROR;
      goto gif_decode_frame_exit;
      }
    offset_x = gifData[1] | (gifData[2] << 8);
    offset_y = gifData[3] | (gifData[4] << 8);
    width = gifData[5] | (gifData[6] << 8);
    height = gifData[7] | (gifData[8] << 8);

    /*  Boundary checking - shouldn't ever happen except unless the data has been modified since initialisation. */
    if ((offset_x + width > mWidth) || (offset_y + height > mHeight)) {
      return_value = GIF_DATA_ERROR;
      goto gif_decode_frame_exit;
      }

    /*  Decode the flags */
    flags = gifData[9];
    colour_table_size = 2 << (flags & GIF_COLOUR_TABLE_SIZE_MASK);
    interlace = flags & GIF_INTERLACE_MASK;

    /*  Move our pointer to the colour table or image data (if no colour table is given) */
    gifData += 10;
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Set up the colour table */
    if (flags & GIF_COLOUR_TABLE_MASK) {
      if (gif_bytes < (int)(3 * colour_table_size)) {
        return_value = GIF_INSUFFICIENT_FRAME_DATA;
        goto gif_decode_frame_exit;
        }
      colour_table = mlocal_colour_table;
      if (!clear_image) {
        for (index = 0; index < colour_table_size; index++) {
          /* Gif colour map contents are r,g,b.
           * We want to pack them bytewise into the colour table, such that the red component
           * is in byte 0 and the alpha component is in * byte 3. */
          unsigned char *entry = (unsigned char *) &colour_table[index];
          entry[0] = gifData[0]; /* r */
          entry[1] = gifData[1]; /* g */
          entry[2] = gifData[2]; /* b */
          entry[3] = 0xff;  /* a */
          gifData += 3;
          }
        }
      else {
        gifData += 3 * colour_table_size;
        }
      gif_bytes = (unsigned int)(gif_end - gifData);
      }
    else
      colour_table = mGlobal_colour_table;

    /*  Check if we've finished */
    if (gif_bytes < 1) {
      return_value = GIF_INSUFFICIENT_FRAME_DATA;
      goto gif_decode_frame_exit;
      }
    else if (gifData[0] == GIF_TRAILER) {
      return_value = GIF_OK;
      goto gif_decode_frame_exit;
      }

    /*  Get the frame data */
    frame_data = (unsigned int*)mPicBuffer;

    /*  If we are clearing the image we just clear, if not decode */
    if (!clear_image) {
      /*  Ensure we have enough data for a 1-byte LZW code size + 1-byte gif trailer */
      if (gif_bytes < 2) {
        return_value = GIF_INSUFFICIENT_FRAME_DATA;
        goto gif_decode_frame_exit;
      /*  If we only have a 1-byte LZW code size + 1-byte gif trailer, we're finished */
        }
      else if ((gif_bytes == 2) && (gifData[1] == GIF_TRAILER)) {
        return_value = GIF_OK;
        goto gif_decode_frame_exit;
        }

      /*  If the previous frame's disposal method requires we restore the background
       *  colour or this is the first frame, clear the frame data */
      if ((frame == 0) || (mdecoded_frame == GIF_INVALID_FRAME)) {
        memset((char*)frame_data, GIF_TRANSPARENT_COLOUR, mWidth * mHeight * sizeof(int));
        mdecoded_frame = frame;
        /* The line below would fill the image with its background color, but because GIFs support
         * transparency we likely wouldn't want to do that. */
        /* memset((char*)frame_data, colour_table[mbackground_index], mWidth * mHeight * sizeof(int)); */
        }
      else if ((frame != 0) && (mFrames[frame - 1].disposal_method == GIF_FRAME_CLEAR)) {
        clear_image = true;
        if ((return_value = decodeBody((frame - 1))) != GIF_OK)
          goto gif_decode_frame_exit;
        clear_image = false;
      /*  If the previous frame's disposal method requires we restore the previous
       *  image, find the last image set to "do not dispose" and get that frame data
      */
        }
      else if ((frame != 0) && (mFrames[frame - 1].disposal_method == GIF_FRAME_RESTORE)) {
        while ((last_undisposed_frame != -1) && (mFrames[--last_undisposed_frame].disposal_method == GIF_FRAME_RESTORE))
          ;

        /*  If we don't find one, clear the frame data */
        if (last_undisposed_frame == -1)
          /* see notes above on transparency vs. background color */
          memset((char*)frame_data, GIF_TRANSPARENT_COLOUR, mWidth * mHeight * sizeof(int));

        else {
          if ((return_value = decodeBody(last_undisposed_frame)) != GIF_OK)
            goto gif_decode_frame_exit;
          /*  Get this frame's data */
          frame_data = (unsigned int*)mPicBuffer;
          }
        }
      mdecoded_frame = frame;

      /*  Initialise the LZW decoding */
      set_code_size = gifData[0];
      buffer_position = (unsigned int)(gifData - mGifData) + 1;

      /*  Set our code variables */
      code_size = set_code_size + 1;
      clear_code = (1 << set_code_size);
      end_code = clear_code + 1;
      max_code_size = clear_code << 1;
      max_code = clear_code + 2;
      curbit = lastbit = 0;
      last_byte = 2;
      get_done = false;
      direct = buf;
      gif_init_LZW();

      /*  Decompress the data */
      for (y = 0; y < height; y++) {
        if (interlace)
          decode_y = gif_interlaced_line(height, y) + offset_y;
        else
          decode_y = y + offset_y;
        frame_scanline = frame_data + offset_x + (decode_y * mWidth);

        /*  Rather than decoding pixel by pixel, we try to burst out streams
          of data to remove the need for end-of data checks every pixel. */
        x = width;
        while (x > 0) {
          burst_bytes = (unsigned int)(mStackpointer - stack);
          if (burst_bytes > 0) {
            if (burst_bytes > x)
              burst_bytes = x;
            x -= burst_bytes;
            while (burst_bytes-- > 0) {
              colour = *--mStackpointer;
              if (((mFrames[frame].transparency) &&
                (colour != mFrames[frame].transparency_index)) ||
                (!mFrames[frame].transparency))
                  *frame_scanline = colour_table[colour];
              frame_scanline++;
              }
            }
          else {
            if (!gif_next_LZW()) {
              /*  Unexpected end of frame, try to recover */
              if (mcurrent_error == GIF_END_OF_FRAME)
                return_value = GIF_OK;
              else
                return_value = mcurrent_error;
              goto gif_decode_frame_exit;
              }
            }
         }
        }
      }
    else {
      /*  Clear our frame */
      if (mFrames[frame].disposal_method == GIF_FRAME_CLEAR) {
        for (y = 0; y < height; y++) {
          frame_scanline = frame_data + offset_x + ((offset_y + y) * mWidth);
          if (mFrames[frame].transparency)
            memset(frame_scanline, GIF_TRANSPARENT_COLOUR, width * 4);
          else
            memset(frame_scanline, colour_table[mbackground_index], width * 4);
          }
        }
      }
  gif_decode_frame_exit:
    /*  Check if we should test for optimisation */
    if (mFrames[frame].virgin) {
      mFrames[frame].opaque = false;
      mFrames[frame].virgin = false;
      }

    buffer_position = save_buffer_position;

    // twiddle RGBA to BGRA
    auto rgbaBuf = (uint32_t*)getPic();
    for (int i = 0; i < getWidth() * getHeight(); i++)
      rgbaBuf[i] = (rgbaBuf[i] & 0xFF00FF00) | ((rgbaBuf[i] & 0x000000FF) << 16) | ((rgbaBuf[i] & 0x00FF0000) >> 16);

    return (eGifResult)return_value;
    }
  //}}}

protected:
  uint8_t* mSrcBuffer = nullptr;
  int mFileSize = 0;

private:
  //{{{  const
  const int maskTbl[16] = {0x0000, 0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f,
                           0x00ff, 0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff};
  //}}}
  //{{{
  eGifResult gif_initialise_sprite (unsigned int width, unsigned int height) {

    unsigned int max_width;
    unsigned int max_height;

    /*  Check if we've changed */
    if ((width <= mWidth) && (height <= mHeight))
      return GIF_OK;

    /*  Get our maximum values */
    max_width = (width > mWidth) ? width : mWidth;
    max_height = (height > mHeight) ? height : mHeight;

    /*  Allocate some more memory */
    mPicBuffer = bigMalloc ((max_width * max_height * 4), "GIFbuffer");
    mWidth = max_width;
    mHeight = max_height;

    /*  Invalidate our currently decoded image */
    mdecoded_frame = GIF_INVALID_FRAME;
    return GIF_OK;
    }
  //}}}
  //{{{
  eGifResult gif_initialise_frame_extensions (const int frame) {

    unsigned char *gifData, *gif_end;
    int gif_bytes;
    unsigned int block_size;

    /*  Get our buffer position etc. */
    gifData = (unsigned char *)(mGifData + buffer_position);
    gif_end = (unsigned char *)(mGifData + buffer_size);

    /*  Initialise the extensions */
    while (gifData < gif_end && gifData[0] == GIF_EXTENSION_INTRODUCER) {
      ++gifData;
      if ((gif_bytes = (unsigned int)(gif_end - gifData)) < 1)
        return GIF_INSUFFICIENT_FRAME_DATA;

      /*  Switch on extension label */
      switch (gifData[0]) {
        /* 6-byte Graphic Control Extension is:
         *  +0  CHAR  Graphic Control Label
         *  +1  CHAR  Block Size
         *  +2  CHAR  __Packed Fields__
         *      3BITS Reserved
         *      3BITS Disposal Method
         *      1BIT  User Input Flag
         *      1BIT  Transparent Color Flag
         *  +3  SHORT Delay Time
         *  +5  CHAR  Transparent Color Index */
        //{{{
        case GIF_EXTENSION_GRAPHIC_CONTROL:
          if (gif_bytes < 6) return GIF_INSUFFICIENT_FRAME_DATA;
          mFrames[frame].frame_delay = gifData[3] | (gifData[4] << 8);
          if (gifData[2] & GIF_TRANSPARENCY_MASK) {
            mFrames[frame].transparency = true;
            mFrames[frame].transparency_index = gifData[5];
            }
          mFrames[frame].disposal_method = ((gifData[2] & GIF_DISPOSAL_MASK) >> 2);
          /*  I have encountered documentation and GIFs in the wild that use
           *  0x04 to restore the previous frame, rather than the officially
           *  documented 0x03.  I believe some (older?) software may even actually
           *  export this way.  We handle this as a type of "quirks" mode. */
          if (mFrames[frame].disposal_method == GIF_FRAME_QUIRKS_RESTORE)
            mFrames[frame].disposal_method = GIF_FRAME_RESTORE;
          gifData += (2 + gifData[1]);
          break;
        //}}}

        /* 14-byte+ Application Extension is:
         *  +0  CHAR  Application Extension Label
         *  +1  CHAR  Block Size
         *  +2  8CHARS  Application Identifier
         *  +10 3CHARS  Appl. Authentication Code
         *  +13 1-256 Application Data (Data sub-blocks) */
        //{{{
        case GIF_EXTENSION_APPLICATION:
          if (gif_bytes < 17) return GIF_INSUFFICIENT_FRAME_DATA;
          if ((gifData[1] == 0x0b) &&
            (strncmp((const char *) gifData + 2,
              "NETSCAPE2.0", 11) == 0) &&
            (gifData[13] == 0x03) &&
            (gifData[14] == 0x01)) {
              mloop_count = gifData[15] | (gifData[16] << 8);
            }
          gifData += (2 + gifData[1]);
          break;
        //}}}

        /*  Move the pointer to the first data sub-block *  Skip 1 byte for the extension label */
        //{{{
        case GIF_EXTENSION_COMMENT:
          ++gifData;
          break;
        //}}}

        /*  Move the pointer to the first data sub-block
         *  Skip 2 bytes for the extension label and size fields *  Skip the extension size itself */
        //{{{
        default:
          if (gif_bytes < 2) return GIF_INSUFFICIENT_FRAME_DATA;
          gifData += (2 + gifData[1]);
        //}}}
        }

      /*  Repeatedly skip blocks until we get a zero block or run out of data  This data is ignored by this gif decoder */
      gif_bytes = (unsigned int)(gif_end - gifData);
      block_size = 0;
      while (gifData < gif_end && gifData[0] != GIF_BLOCK_TERMINATOR) {
        block_size = gifData[0] + 1;
        if ((gif_bytes -= block_size) < 0)
          return GIF_INSUFFICIENT_FRAME_DATA;
        gifData += block_size;
        }
      ++gifData;
      }

    /*  Set buffer position and return */
    buffer_position = (unsigned int)(gifData - mGifData);
    return GIF_OK;
    }
  //}}}
  //{{{
  eGifResult gif_initialise_frame () {

    int frame;
    tGifFrame *temp_buf;

    unsigned char *gifData, *gif_end;
    int gif_bytes;
    unsigned int flags = 0;
    unsigned int width, height, offset_x, offset_y;
    unsigned int block_size, colour_table_size;
    bool first_image = true;
    eGifResult return_value;

    /*  Get the frame to decode and our data position */
    frame = mFrame_count;

    /*  Get our buffer position etc. */
    gifData = (unsigned char *)(mGifData + buffer_position);
    gif_end = (unsigned char *)(mGifData + buffer_size);
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Check if we've finished */
    if ((gif_bytes > 0) && (gifData[0] == GIF_TRAILER))
      return GIF_OK;

    /*  Check if we have enough data
     *  The shortest block of data is a 4-byte comment extension + 1-byte block terminator + 1-byte gif trailer */
    if (gif_bytes < 6)
      return GIF_INSUFFICIENT_DATA;

    /*  We could theoretically get some junk data that gives us millions of frames, ensure  we don't have  silly number */
    if (frame > 4096)
      return GIF_FRAME_DATA_ERROR;

    /*  Get some memory to store our pointers in etc.*/
    if ((int)mFrame_holders <= frame) {
      /*  Allocate more memory */
      if ((temp_buf = (tGifFrame*)realloc (mFrames, (frame + 1) * sizeof(tGifFrame))) == NULL)
        return GIF_INSUFFICIENT_MEMORY;
      mFrames = temp_buf;
      mFrame_holders = frame + 1;
      }

    /*  Store our frame pointer. We would do it when allocating except we
      start off with one frame allocated so we can always use realloc. */
    mFrames[frame].frame_pointer = buffer_position;
    mFrames[frame].display = false;
    mFrames[frame].virgin = true;
    mFrames[frame].disposal_method = 0;
    mFrames[frame].transparency = false;
    mFrames[frame].frame_delay = 100;
    mFrames[frame].redraw_required = false;

    /*  Invalidate any previous decoding we have of this frame */
    if (mdecoded_frame == frame)
      mdecoded_frame = GIF_INVALID_FRAME;

    /*  We pretend to initialise the frames, but really we just skip over all
      the data contained within. This is all basically a cut down version of
      gif_decode_frame that doesn't have any of the LZW bits in it. */

    /*  Initialise any extensions */
    buffer_position = (unsigned int)(gifData - mGifData);
    if ((return_value = gif_initialise_frame_extensions(frame)) != GIF_OK)
      return return_value;
    gifData = (mGifData + buffer_position);
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Check if we've finished */
    if ((gif_bytes = (unsigned int)(gif_end - gifData)) < 1)
      return GIF_INSUFFICIENT_FRAME_DATA;
    else if (gifData[0] == GIF_TRAILER) {
      buffer_position = (unsigned int)(gifData - mGifData);
      mFrame_count = frame + 1;
      return GIF_OK;
      }

    /*  If we're not done, there should be an image descriptor */
    if (gifData[0] != GIF_IMAGE_SEPARATOR)
      return GIF_FRAME_DATA_ERROR;

    /*  Do some simple boundary checking */
    if (gif_bytes < 10)
      return GIF_INSUFFICIENT_FRAME_DATA;
    offset_x = gifData[1] | (gifData[2] << 8);
    offset_y = gifData[3] | (gifData[4] << 8);
    width = gifData[5] | (gifData[6] << 8);
    height = gifData[7] | (gifData[8] << 8);

    /*  Set up the redraw characteristics. We have to check for extending the area due to multi-image frames. */
    if (!first_image) {
      if (mFrames[frame].redraw_x > offset_x) {
        mFrames[frame].redraw_width += (mFrames[frame].redraw_x - offset_x);
        mFrames[frame].redraw_x = offset_x;
        }
      if (mFrames[frame].redraw_y > offset_y) {
        mFrames[frame].redraw_height += (mFrames[frame].redraw_y - offset_y);
        mFrames[frame].redraw_y = offset_y;
        }
      if ((offset_x + width) > (mFrames[frame].redraw_x + mFrames[frame].redraw_width))
        mFrames[frame].redraw_width = (offset_x + width) - mFrames[frame].redraw_x;
      if ((offset_y + height) > (mFrames[frame].redraw_y + mFrames[frame].redraw_height))
        mFrames[frame].redraw_height = (offset_y + height) - mFrames[frame].redraw_y;
      }
    else {
      first_image = false;
      mFrames[frame].redraw_x = offset_x;
      mFrames[frame].redraw_y = offset_y;
      mFrames[frame].redraw_width = width;
      mFrames[frame].redraw_height = height;
      }

    /*  if we are clearing the background then we need to redraw enough to cover the previous frame too */
    mFrames[frame].redraw_required = ((mFrames[frame].disposal_method == GIF_FRAME_CLEAR) ||
                                      (mFrames[frame].disposal_method == GIF_FRAME_RESTORE));

    /*  Boundary checking - shouldn't ever happen except with junk data */
    if (gif_initialise_sprite((offset_x + width), (offset_y + height)))
      return GIF_INSUFFICIENT_MEMORY;

    /*  Decode the flags */
    flags = gifData[9];
    colour_table_size = 2 << (flags & GIF_COLOUR_TABLE_SIZE_MASK);

    /*  Move our data onwards and remember we've got a bit of this frame */
    gifData += 10;
    gif_bytes = (unsigned int)(gif_end - gifData);
    mFrame_count_partial = frame + 1;

    /*  Skip the local colour table */
    if (flags & GIF_COLOUR_TABLE_MASK) {
      gifData += 3 * colour_table_size;
      if ((gif_bytes = (unsigned int)(gif_end - gifData)) < 0)
        return GIF_INSUFFICIENT_FRAME_DATA;
      }

    /*  Ensure we have a correct code size */
    if (gif_bytes < 1)
      return GIF_INSUFFICIENT_FRAME_DATA;
    if (gifData[0] > GIF_MAX_LZW)
      return GIF_DATA_ERROR;

    /*  Move our pointer to the actual image data */
    gifData++;
    --gif_bytes;

    /*  Repeatedly skip blocks until we get a zero block or run out of data
     *  These blocks of image data are processed later by gif_decode_frame() */
    block_size = 0;
    while (block_size != 1) {
      if (gif_bytes < 1) return
        GIF_INSUFFICIENT_FRAME_DATA;
      block_size = gifData[0] + 1;
      /*  Check if the frame data runs off the end of the file */
      if ((int)(gif_bytes - block_size) < 0) {
        /*  Try to recover by signaling the end of the gif.
         *  Once we get garbage data, there is no logical *  way to determine where the next frame is.
         *  It's probably better to partially load the gif *  than not at all. */
        if (gif_bytes >= 2) {
          gifData[0] = 0;
          gifData[1] = GIF_TRAILER;
          gif_bytes = 1;
          ++gifData;
          break;
          }
        else
          return GIF_INSUFFICIENT_FRAME_DATA;
        }
      else {
        gif_bytes -= block_size;
        gifData += block_size;
        }
      }

    /*  Add the frame and set the display flag */
    buffer_position = (unsigned int)(gifData - mGifData);
    mFrame_count = frame + 1;
    mFrames[frame].display = true;

    /*  Check if we've finished */
    if (gif_bytes < 1)
      return GIF_INSUFFICIENT_FRAME_DATA;
    else if (gifData[0] == GIF_TRAILER)
      return GIF_OK;

    return GIF_WORKING;
    }
  //}}}
  //{{{
  unsigned int gif_interlaced_line (int height, int y) {

    if ((y << 3) < height)
      return (y << 3);

    y -= ((height + 7) >> 3);
    if ((y << 3) < (height - 4))
      return (y << 3) + 4;

    y -= ((height + 3) >> 3);
    if ((y << 2) < (height - 2))
      return (y << 2) + 2;

    y -= ((height + 1) >> 2);
    return (y << 1) + 1;
    }
  //}}}
  //{{{
  eGifResult gif_skip_frame_extensions () {
  /** Skips the frame's extensions (which have been previously initialised)
    @return GIF_INSUFFICIENT_FRAME_DATA for insufficient data to complete the frame GIF_OK for successful decoding */
    unsigned char *gifData, *gif_end;
    int gif_bytes;
    unsigned int block_size;

    /*  Get our buffer position etc. */
    gifData = (unsigned char *)(mGifData + buffer_position);
    gif_end = (unsigned char *)(mGifData + buffer_size);
    gif_bytes = (unsigned int)(gif_end - gifData);

    /*  Skip the extensions */
    while (gifData < gif_end && gifData[0] == GIF_EXTENSION_INTRODUCER) {
      ++gifData;
      if (gifData >= gif_end)
        return GIF_INSUFFICIENT_FRAME_DATA;

      /*  Switch on extension label */
      switch(gifData[0]) {
        /*  Move the pointer to the first data sub-block *  1 byte for the extension label */
        case GIF_EXTENSION_COMMENT:
          ++gifData;
          break;

        /*  Move the pointer to the first data sub-block
         *  2 bytes for the extension label and size fields *  Skip the extension size itself */
        default:
          if (gifData + 1 >= gif_end)
            return GIF_INSUFFICIENT_FRAME_DATA;
          gifData += (2 + gifData[1]);
        }

      /*  Repeatedly skip blocks until we get a zero block or run out of data
       *  This data is ignored by this gif decoder */
      gif_bytes = (unsigned int)(gif_end - gifData);
      block_size = 0;
      while (gifData < gif_end && gifData[0] != GIF_BLOCK_TERMINATOR) {
        block_size = gifData[0] + 1;
        if ((gif_bytes -= block_size) < 0)
          return GIF_INSUFFICIENT_FRAME_DATA;
        gifData += block_size;
        }
      ++gifData;
      }

    /*  Set buffer position and return */
    buffer_position = (unsigned int)(gifData - mGifData);
    return GIF_OK;
    }
  //}}}
  //{{{
  int gif_next_code (int code_size) {

    int i, j, end, count, ret;
    unsigned char *b;

    end = curbit + code_size;
    if (end >= lastbit) {
      if (get_done)
        return GIF_END_OF_FRAME;
      buf[0] = direct[last_byte - 2];
      buf[1] = direct[last_byte - 1];

      /* get the next block */
      direct = mGifData + buffer_position;
      if (buffer_position >= buffer_size)
        return GIF_INSUFFICIENT_FRAME_DATA;
      zero_data_block = ((count = direct[0]) == 0);
      if ((buffer_position + count) >= buffer_size)
        return GIF_INSUFFICIENT_FRAME_DATA;
      if (count == 0)
        get_done = true;
      else {
        if (buffer_position + 3 >= buffer_size)
          return GIF_INSUFFICIENT_FRAME_DATA;
        direct -= 1;
        buf[2] = direct[2];
        buf[3] = direct[3];
        }
      buffer_position += count + 1;

      /* update our variables */
      last_byte = 2 + count;
      curbit = (curbit - lastbit) + 16;
      lastbit = (2 + count) << 3;
      end = curbit + code_size;
      }

    i = curbit >> 3;
    if (i < 2)
      b = buf;
    else
      b = direct;

    ret = b[i];
    j = (end >> 3) - 1;
    if (i <= j) {
      ret |= (b[i + 1] << 8);
      if (i < j)
        ret |= (b[i + 2] << 16);
      }
    ret = (ret >> (curbit % 8)) & maskTbl[code_size];

    curbit += code_size;
    return ret;
    }
  //}}}
  //{{{
  void gif_init_LZW () {

    int i;
    mcurrent_error = (eGifResult)0;
    if (clear_code >= (1 << GIF_MAX_LZW)) {
      mStackpointer = stack;
      mcurrent_error = GIF_FRAME_DATA_ERROR;
      return;
      }

    /* initialise our table */
    memset(table, 0x00, (1 << GIF_MAX_LZW) * 8);
    for (i = 0; i < clear_code; ++i)
      table[1][i] = i;

    /* update our LZW parameters */
    code_size = set_code_size + 1;
    max_code_size = clear_code << 1;
    max_code = clear_code + 2;
    mStackpointer = stack;
    do {
      firstcode = oldcode = gif_next_code(code_size);
      } while (firstcode == clear_code);

    *mStackpointer++ =firstcode;
    }
  //}}}
  //{{{
  bool gif_next_LZW () {

    int code, incode;
    int block_size;
    int new_code;

    code = gif_next_code(code_size);
    if (code < 0) {
        mcurrent_error = (eGifResult)code;
      return false;
      }
    else if (code == clear_code) {
      gif_init_LZW();
      return true;
      }
    else if (code == end_code) {
      /* skip to the end of our data so multi-image GIFs work */
      if (zero_data_block) {
        mcurrent_error = GIF_FRAME_DATA_ERROR;
        return false;
        }

      block_size = 0;
      while (block_size != 1 &&
          buffer_position < buffer_size) {
        block_size = mGifData[buffer_position] + 1;
        buffer_position += block_size;
        }
      mcurrent_error = GIF_FRAME_DATA_ERROR;
      return false;
      }

    incode = code;
    if (code >= max_code) {
      if (mStackpointer >= stack + ((1 << GIF_MAX_LZW) * 2)) {
        mcurrent_error = GIF_FRAME_DATA_ERROR;
        return false;
        }
      *mStackpointer++ = firstcode;
      code = oldcode;
      }

    /* The following loop is the most important in the GIF decoding cycle as every
     * single pixel passes through it.
     * Note: our stack is always big enough to hold a complete decompressed chunk. */
    while (code >= clear_code) {
      if (mStackpointer >= stack + ((1 << GIF_MAX_LZW) * 2) ||
          code >= (1 << GIF_MAX_LZW)) {
        mcurrent_error = GIF_FRAME_DATA_ERROR;
        return false;
        }
      *mStackpointer++ = table[1][code];
      new_code = table[0][code];
      if (new_code < clear_code) {
        code = new_code;
        break;
        }

      if (mStackpointer >= stack + ((1 << GIF_MAX_LZW) * 2) ||
          new_code >= (1 << GIF_MAX_LZW)) {
        mcurrent_error = GIF_FRAME_DATA_ERROR;
        return false;
        }
      *mStackpointer++ = table[1][new_code];
      code = table[0][new_code];
      if (code == new_code) {
          mcurrent_error = GIF_FRAME_DATA_ERROR;
        return false;
        }
      }

    if (mStackpointer >= stack + ((1 << GIF_MAX_LZW) * 2)) {
      mcurrent_error = GIF_FRAME_DATA_ERROR;
      return false;
      }
    *mStackpointer++ = firstcode = table[1][code];

    if ((code = max_code) < (1 << GIF_MAX_LZW)) {
      table[0][code] = oldcode;
      table[1][code] = firstcode;
      ++max_code;
      if ((max_code >= max_code_size) && (max_code_size < (1 << GIF_MAX_LZW))) {
        max_code_size = max_code_size << 1;
        ++code_size;
        }
      }

    oldcode = incode;
    return true;
    }
  //}}}

  //{{{  vars
  unsigned int mWidth;
  unsigned int mHeight;
  void* mPicBuffer;

  unsigned int mFrame_count;         /**< number of frames decoded */
  unsigned int mFrame_count_partial; /**< number of frames partially decoded */
  int mdecoded_frame = GIF_INVALID_FRAME; /**< current frame decoded to bitmap */
  int mloop_count;                   /**< number of times to loop animation */

  //{{{
   struct tGifFrame {
    bool display;                     /**< whether the frame should be displayed/animated */
    unsigned int frame_delay;         /**< delay (in cs) before animating the frame */

    // Internal members
    unsigned int frame_pointer;       /**< offset (in bytes) to the GIF frame data */
    bool virgin;                      /**< whether the frame has previously been used */
    bool opaque;                      /**< whether the frame is totally opaque */
    bool redraw_required;             /**< whether a forcable screen redraw is required */
    unsigned char disposal_method;    /**< how the previous frame should be disposed; affects plotting */
    bool transparency;                /**< whether we acknoledge transparency */
    unsigned char transparency_index; /**< the index designating a transparent pixel */
    unsigned int redraw_x;            /**< x co-ordinate of redraw rectangle */
    unsigned int redraw_y;            /**< y co-ordinate of redraw rectangle */
    unsigned int redraw_width;        /**< width of redraw rectangle */
    unsigned int redraw_height;       /**< height of redraw rectangle */
    };
  //}}}
  tGifFrame* mFrames;                /**< decoded frames */

  eGifResult mcurrent_error;         /**< current error type, or 0 for none*/
  unsigned char* mGifData;           /**< pointer to GIF data */
  unsigned int buffer_size = 0;      /**< total number of bytes of GIF data available */
  unsigned int buffer_position = 0;  /**< current index into GIF data */

  unsigned char buf[4];
  unsigned char* direct;
  int table[2][(1 << GIF_MAX_LZW)];
  unsigned char  stack[(1 << GIF_MAX_LZW) * 2];
  unsigned char* mStackpointer;

  int code_size, set_code_size;
  int max_code, max_code_size;
  int clear_code, end_code;
  int curbit, lastbit, last_byte;
  int firstcode, oldcode;
  bool zero_data_block = false;
  bool get_done;
  bool clear_image = false;

  unsigned int  mFrame_holders;       /**< current number of frame holders */
  unsigned int  mbackground_index;    /**< index in the colour table for the background colour */
  unsigned int  maspect_ratio;        /**< image aspect ratio (ignored) */
  unsigned int  mcolour_table_size;   /**< size of colour table (in entries) */
  bool          mGlobal_colours;      /**< whether the GIF has a global colour table */
  unsigned int* mGlobal_colour_table; /**< global colour table */
  unsigned int* mlocal_colour_table;  /**< local colour table */
  //}}}
  };
