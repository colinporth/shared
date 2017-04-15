//{{{
void drawEditBoxBase (NVGcontext* vg, float x, float y, float w, float h) {

  // Edit
  auto bg = nvgBoxGradient (vg, x+1,y+1+1.5f, w-2,h-2, 3,4, nvgRGBA(255,255,255,32), nvgRGBA(32,32,32,32));
  nvgBeginPath (vg);
  nvgRoundedRect (vg, x+1,y+1, w-2,h-2, 4-1);
  nvgFillPaint (vg, bg);
  nvgFill (vg);

  nvgBeginPath (vg);
  nvgRoundedRect (vg, x+0.5f,y+0.5f, w-1,h-1, 4-0.5f);
  nvgStrokeColor (vg, nvgRGBA(0,0,0,48));
  nvgStroke (vg);
  }
//}}}
//{{{
void drawParagraph (NVGcontext* vg, float x, float y, float width, float height, float cursorX, float cursorY) {

  NVGtextRow rows[3];
  NVGglyphPosition glyphs[100];
  const char* text = "This is longer chunk of text.\n  \n  Would have used lorem ipsum but she    was busy jumping over the lazy dog with the fox and all the men who came to the aid of the party.🎉";
  const char* start;
  const char* end;
  int nrows, i, nglyphs, j, lnum = 0;
  float lineh;
  float caretx, px;
  float bounds[4];
  float a;
  float gx,gy;
  int gutter = 0;
  NVG_NOTUSED(height);

  nvgSave(vg);

  nvgFontSize (vg, 18.0f);
  nvgFontFace (vg, "sans");
  nvgTextAlign (vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
  nvgTextMetrics (vg, NULL, NULL, &lineh);

  // The text break API can be used to fill a large buffer of rows,
  // or to iterate over the text just few lines (or just one) at a time.
  // The "next" variable of the last returned item tells where to continue.
  start = text;
  end = text + strlen (text);
  while ((nrows = nvgTextBreakLines (vg, start, end, width, rows, 3))) {
    for (i = 0; i < nrows; i++) {
      auto row = &rows[i];
      int hit = cursorX > x && cursorX < (x+width) && cursorY >= y && cursorY < (y+lineh);

      nvgBeginPath (vg);
      nvgFillColor (vg, nvgRGBA(255,255,255,hit?64:16));
      nvgRect (vg, x, y, row->width, lineh);
      nvgFill (vg);

      nvgFillColor (vg, nvgRGBA(255,255,255,255));
      nvgText (vg, x, y, row->start, row->end);

      if (hit) {
        caretx = (cursorX < x+row->width/2) ? x : x+row->width;
        px = x;
        nglyphs = nvgTextGlyphPositions (vg, x, y, row->start, row->end, glyphs, 100);
        for (j = 0; j < nglyphs; j++) {
          float x0 = glyphs[j].x;
          float x1 = (j+1 < nglyphs) ? glyphs[j+1].x : x+row->width;
          float gx = x0 * 0.3f + x1 * 0.7f;
          if (cursorX >= px && cursorX < gx)
            caretx = glyphs[j].x;
          px = gx;
        }
        nvgBeginPath (vg);
        nvgFillColor (vg, nvgRGBA(255,192,0,255));
        nvgRect (vg, caretx, y, 1, lineh);
        nvgFill (vg);

        gutter = lnum+1;
        gx = x - 10;
        gy = y + lineh/2;
        }
      lnum++;
      y += lineh;
      }
    // Keep going...
     start = rows[nrows-1].next;
   }

  if (gutter) {
    char txt[16];
    snprintf (txt, sizeof(txt), "%d", gutter);
    nvgFontSize (vg, 13.0f);
    nvgTextAlign (vg, NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);

    nvgTextBounds (vg, gx,gy, txt, NULL, bounds);

    nvgBeginPath (vg);
    nvgFillColor (vg, nvgRGBA(255,192,0,255));
    nvgRoundedRect (vg, (float)(int)bounds[0]-4, (float)(int)bounds[1]-2, (float)(int)(bounds[2]-bounds[0])+8, (float)(int)(bounds[3]-bounds[1])+4, (float)((int)(bounds[3]-bounds[1])+4)/2-1);
    nvgFill (vg);

    nvgFillColor (vg, nvgRGBA(32,32,32,255));
    nvgText (vg, gx,gy, txt, NULL);
    }

  y += 20.0f;

  nvgFontSize (vg, 13.0f);
  nvgTextAlign (vg, NVG_ALIGN_LEFT|NVG_ALIGN_TOP);
  nvgTextLineHeight (vg, 1.2f);

  nvgTextBoxBounds (vg, x,y, 150, "Hover your mouse over the text to see calculated caret position.", NULL, bounds);

  // Fade the tooltip out when close to it.
  gx = fabsf((cursorX - (bounds[0]+bounds[2])*0.5f) / (bounds[0] - bounds[2]));
  gy = fabsf((cursorY - (bounds[1]+bounds[3])*0.5f) / (bounds[1] - bounds[3]));
  a = maxf (gx, gy) - 0.5f;
  a = clampf (a, 0, 1);
  nvgGlobalAlpha (vg, a);

  nvgBeginPath (vg);
  nvgFillColor (vg, nvgRGBA(220,220,220,255));
  nvgRoundedRect (vg, bounds[0]-2,bounds[1]-2, (float)(int)(bounds[2]-bounds[0])+4, (float)(int)(bounds[3]-bounds[1])+4, 3);
  px = (float)(int)((bounds[2]+bounds[0])/2);
  nvgMoveTo (vg, px,bounds[1] - 10);
  nvgLineTo (vg, px+7,bounds[1]+1);
  nvgLineTo (vg, px-7,bounds[1]+1);
  nvgFill (vg);

  nvgFillColor (vg, nvgRGBA(0,0,0,220));
  nvgTextBox (vg, x,y, 150, "Hover your mouse over the text to see calculated caret position.", NULL);

  nvgRestore (vg);
  }
//}}}
//{{{
void drawColorwheel (NVGcontext* vg, float x, float y, float w, float h, float t) {

  float r0, r1, ax,ay, bx,by, cx,cy, aeps, r;
  float hue = sinf(t * 0.12f);

  nvgSave (vg);

  /*nvgBeginPath(vg);
  nvgRect(vg, x,y,w,h);
  nvgFillColor(vg, nvgRGBA(255,0,0,128));
  nvgFill(vg);*/

  cx = x + w*0.5f;
  cy = y + h*0.5f;
  r1 = (w < h ? w : h) * 0.5f - 5.0f;
  r0 = r1 - 20.0f;
  aeps = 0.5f / r1; // half a pixel arc length in radians (2pi cancels out).

  for (int i = 0; i < 6; i++) {
    float a0 = (float)i / 6.0f * NVG_PI * 2.0f - aeps;
    float a1 = (float)(i+1.0f) / 6.0f * NVG_PI * 2.0f + aeps;
    nvgBeginPath (vg);
    nvgArc (vg, cx,cy, r0, a0, a1, NVG_CW);
    nvgArc (vg, cx,cy, r1, a1, a0, NVG_CCW);
    nvgClosePath (vg);
    ax = cx + cosf(a0) * (r0+r1)*0.5f;
    ay = cy + sinf(a0) * (r0+r1)*0.5f;
    bx = cx + cosf(a1) * (r0+r1)*0.5f;
    by = cy + sinf(a1) * (r0+r1)*0.5f;
    auto paint = nvgLinearGradient (vg, ax,ay, bx,by, nvgHSLA(a0/(NVG_PI*2),1.0f,0.55f,255), nvgHSLA(a1/(NVG_PI*2),1.0f,0.55f,255));
    nvgFillPaint (vg, paint);
    nvgFill (vg);
    }

  nvgBeginPath (vg);
  nvgCircle (vg, cx,cy, r0-0.5f);
  nvgCircle (vg, cx,cy, r1+0.5f);
  nvgStrokeColor (vg, nvgRGBA(0,0,0,64));
  nvgStrokeWidth (vg, 1.0f);
  nvgStroke (vg);

  // Selector
  nvgSave (vg);
  nvgTranslate (vg, cx,cy);
  nvgRotate (vg, hue*NVG_PI*2);

  // Marker on
  nvgStrokeWidth (vg, 2.0f);
  nvgBeginPath (vg);
  nvgRect (vg, r0-1,-3,r1-r0+2,6);
  nvgStrokeColor (vg, nvgRGBA(255,255,255,192));
  nvgStroke (vg);

  auto paint = nvgBoxGradient (vg, r0-3,-5,r1-r0+6,10, 2,4, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
  nvgBeginPath (vg);
  nvgRect (vg, r0-2-10,-4-10,r1-r0+4+20,8+20);
  nvgRect (vg, r0-2,-4,r1-r0+4,8);
  nvgPathWinding (vg, NVG_HOLE);
  nvgFillPaint (vg, paint);
  nvgFill (vg);

  // Center triangle
  r = r0 - 6;
  ax = cosf(120.0f/180.0f*NVG_PI) * r;
  ay = sinf(120.0f/180.0f*NVG_PI) * r;
  bx = cosf(-120.0f/180.0f*NVG_PI) * r;
  by = sinf(-120.0f/180.0f*NVG_PI) * r;
  nvgBeginPath(vg);
  nvgMoveTo(vg, r,0);
  nvgLineTo(vg, ax,ay);
  nvgLineTo(vg, bx,by);
  nvgClosePath(vg);
  paint = nvgLinearGradient(vg, r,0, ax,ay, nvgHSLA(hue,1.0f,0.5f,255), nvgRGBA(255,255,255,255));
  nvgFillPaint(vg, paint);
  nvgFill(vg);
  paint = nvgLinearGradient(vg, (r+ax)*0.5f,(0+ay)*0.5f, bx,by, nvgRGBA(0,0,0,0), nvgRGBA(0,0,0,255));
  nvgFillPaint(vg, paint);
  nvgFill(vg);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,64));
  nvgStroke(vg);

  // Select circle on triangle
  ax = cosf(120.0f/180.0f*NVG_PI) * r*0.3f;
  ay = sinf(120.0f/180.0f*NVG_PI) * r*0.4f;
  nvgStrokeWidth(vg, 2.0f);
  nvgBeginPath(vg);
  nvgCircle(vg, ax,ay,5);
  nvgStrokeColor(vg, nvgRGBA(255,255,255,192));
  nvgStroke(vg);

  paint = nvgRadialGradient(vg, ax,ay, 7,9, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
  nvgBeginPath(vg);
  nvgRect(vg, ax-20,ay-20,40,40);
  nvgCircle(vg, ax,ay,7);
  nvgPathWinding(vg, NVG_HOLE);
  nvgFillPaint(vg, paint);
  nvgFill(vg);

  nvgRestore(vg);
  nvgRestore(vg);
  }
//}}}
//{{{
void drawWidths (NVGcontext* vg, float x, float y, float width) {

  nvgSave (vg);

  nvgStrokeColor (vg, nvgRGBA (0,0,0,255));
  for (int i = 0; i < 20; i++) {
    float w = (i+0.5f)*0.1f;
    nvgStrokeWidth (vg, w);
    nvgBeginPath (vg);
    nvgMoveTo (vg, x,y);
    nvgLineTo (vg, x+width,y+width*0.3f);
    nvgStroke (vg);
    y += 10;
    }

  nvgRestore (vg);
  }
//}}}
//{{{
void drawCaps (NVGcontext* vg, float x, float y, float width) {

  int i;
  int caps[3] = {NVG_BUTT, NVG_ROUND, NVG_SQUARE};
  float lineWidth = 8.0f;

  nvgSave (vg);

  nvgBeginPath (vg);
  nvgRect (vg, x-lineWidth/2, y, width+lineWidth, 40);
  nvgFillColor (vg, nvgRGBA (255,255,255,32));
  nvgFill (vg);

  nvgBeginPath (vg);
  nvgRect (vg, x, y, width, 40);
  nvgFillColor (vg, nvgRGBA(255,255,255,32));
  nvgFill (vg);

  nvgStrokeWidth (vg, lineWidth);
  for (i = 0; i < 3; i++) {
    nvgLineCap (vg, caps[i]);
    nvgStrokeColor (vg, nvgRGBA(0,0,0,255));
    nvgBeginPath (vg);
    nvgMoveTo (vg, x, y + i*10 + 5);
    nvgLineTo (vg, x+width, y + i*10 + 5);
    nvgStroke (vg);
    }

  nvgRestore (vg);
  }
//}}}
//{{{
void drawWindow (NVGcontext* vg, const char* title, float x, float y, float w, float h) {

  float cornerRadius = 3.0f;

  nvgSave(vg);
  //nvgClearState(vg);

  // Window
  nvgBeginPath(vg);
  nvgRoundedRect (vg, x,y, w,h, cornerRadius);
  nvgFillColor (vg, nvgRGBA(28,30,34,192));
  //nvgFillColor(vg, nvgRGBA(0,0,0,128));
  nvgFill (vg);

  // Drop shadow
  auto shadowPaint = nvgBoxGradient (vg, x,y+2, w,h, cornerRadius*2, 10, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
  nvgBeginPath (vg);
  nvgRect (vg, x-10,y-10, w+20,h+30);
  nvgRoundedRect (vg, x,y, w,h, cornerRadius);
  nvgPathWinding (vg, NVG_HOLE);
  nvgFillPaint (vg, shadowPaint);
  nvgFill (vg);

  // Header
  auto headerPaint = nvgLinearGradient (vg, x,y,x,y+15, nvgRGBA(255,255,255,8), nvgRGBA(0,0,0,16));
  nvgBeginPath (vg);
  nvgRoundedRect (vg, x+1,y+1, w-2,30, cornerRadius-1);
  nvgFillPaint (vg, headerPaint);
  nvgFill (vg);
  nvgBeginPath (vg);
  nvgMoveTo (vg, x+0.5f, y+0.5f+30);
  nvgLineTo (vg, x+0.5f+w-1, y+0.5f+30);
  nvgStrokeColor (vg, nvgRGBA(0,0,0,32));
  nvgStroke (vg);

  nvgFontSize (vg, 18.0f);
  nvgFontFace (vg, "sans-bold");
  nvgTextAlign (vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);

  nvgFontBlur (vg,2);
  nvgFillColor (vg, nvgRGBA(0,0,0,128));
  nvgText (vg, x+w/2,y+16+1, title, NULL);

  nvgFontBlur (vg,0);
  nvgFillColor (vg, nvgRGBA(220,220,220,160));
  nvgText (vg, x+w/2,y+16, title, NULL);

  nvgRestore (vg);
  }
//}}}
//{{{
void drawSearchBox (NVGcontext* vg, const char* text, float x, float y, float w, float h) {

  char icon[8];
  float cornerRadius = h/2-1;

  // Edit
  auto bg = nvgBoxGradient (vg, x,y+1.5f, w,h, h/2,5, nvgRGBA(0,0,0,16), nvgRGBA(0,0,0,92));
  nvgBeginPath (vg);
  nvgRoundedRect (vg, x,y, w,h, cornerRadius);
  nvgFillPaint (vg, bg);
  nvgFill (vg);

/*  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,48));
  nvgStroke(vg);*/

  nvgFontSize (vg, h*1.3f);
  nvgFontFace (vg, "icons");
  nvgFillColor (vg, nvgRGBA(255,255,255,64));
  nvgTextAlign (vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
  nvgText (vg, x+h*0.55f, y+h*0.55f, cpToUTF8(ICON_SEARCH,icon), NULL);

  nvgFontSize (vg, 20.0f);
  nvgFontFace (vg, "sans");
  nvgFillColor (vg, nvgRGBA(255,255,255,32));

  nvgTextAlign (vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgText (vg, x+h*1.05f,y+h*0.5f,text, NULL);

  nvgFontSize (vg, h*1.3f);
  nvgFontFace (vg, "icons");
  nvgFillColor (vg, nvgRGBA(255,255,255,32));
  nvgTextAlign (vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
  nvgText (vg, x+w-h*0.55f, y+h*0.55f, cpToUTF8(ICON_CIRCLED_CROSS,icon), NULL);
  }
//}}}
//{{{
void drawDropDown (NVGcontext* vg, const char* text, float x, float y, float w, float h) {

  char icon[8];
  float cornerRadius = 4.0f;

  auto bg = nvgLinearGradient(vg, x,y,x,y+h, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,48));
  nvgStroke(vg);

  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,160));
  nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+h*0.3f,y+h*0.5f,text, NULL);

  nvgFontSize(vg, h*1.3f);
  nvgFontFace(vg, "icons");
  nvgFillColor(vg, nvgRGBA(255,255,255,64));
  nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+w-h*0.5f, y+h*0.5f, cpToUTF8(ICON_CHEVRON_RIGHT,icon), NULL);
  }
//}}}
//{{{
void drawLabel (NVGcontext* vg, const char* text, float x, float y, float w, float h) {

  NVG_NOTUSED(w);

  nvgFontSize(vg, 18.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,128));

  nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x,y+h*0.5f,text, NULL);
  }
//}}}
//{{{
void drawEditBox (NVGcontext* vg, const char* text, float x, float y, float w, float h) {

  drawEditBoxBase(vg, x,y, w,h);
  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,64));
  nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+h*0.3f,y+h*0.5f,text, NULL);
  }
//}}}
//{{{
void drawEditBoxNum (NVGcontext* vg, const char* text, const char* units, float x, float y, float w, float h) {

  drawEditBoxBase(vg, x,y, w,h);

  float uw = nvgTextBounds(vg, 0,0, units, NULL, NULL);
  nvgFontSize(vg, 18.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,64));
  nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+w-h*0.3f,y+h*0.5f,units, NULL);

  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,128));
  nvgTextAlign(vg,NVG_ALIGN_RIGHT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+w-uw-h*0.5f,y+h*0.5f,text, NULL);
  }
//}}}
//{{{
void drawCheckBox (NVGcontext* vg, const char* text, float x, float y, float w, float h) {

  char icon[8];
  NVG_NOTUSED(w);

  nvgFontSize(vg, 18.0f);
  nvgFontFace(vg, "sans");
  nvgFillColor(vg, nvgRGBA(255,255,255,160));

  nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+28,y+h*0.5f,text, NULL);

  auto bg = nvgBoxGradient(vg, x+1,y+(int)(h*0.5f)-9+1, 18,18, 3,3, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,92));
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+1,y+(int)(h*0.5f)-9, 18,18, 3);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  nvgFontSize(vg, 40);
  nvgFontFace(vg, "icons");
  nvgFillColor(vg, nvgRGBA(255,255,255,128));
  nvgTextAlign(vg,NVG_ALIGN_CENTER|NVG_ALIGN_MIDDLE);
  nvgText(vg, x+9+2, y+h*0.5f, cpToUTF8(ICON_CHECK,icon), NULL);
  }
//}}}
//{{{
void drawButton (NVGcontext* vg, int preicon, const char* text, float x, float y, float w, float h, NVGcolor col) {

  char icon[8];
  float cornerRadius = 4.0f;
  float tw = 0, iw = 0;

  auto bg = nvgLinearGradient(vg, x,y,x,y+h, nvgRGBA(255,255,255,isBlack(col)?16:32), nvgRGBA(0,0,0,isBlack(col)?16:32));
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+1,y+1, w-2,h-2, cornerRadius-1);
  if (!isBlack(col)) {
    nvgFillColor(vg, col);
    nvgFill(vg);
    }
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgRoundedRect(vg, x+0.5f,y+0.5f, w-1,h-1, cornerRadius-0.5f);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,48));
  nvgStroke(vg);

  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans-bold");
  tw = nvgTextBounds(vg, 0,0, text, NULL, NULL);
  if (preicon != 0) {
    nvgFontSize(vg, h*1.3f);
    nvgFontFace(vg, "icons");
    iw = nvgTextBounds(vg, 0,0, cpToUTF8(preicon,icon), NULL, NULL);
    iw += h*0.15f;
    }

  if (preicon != 0) {
    nvgFontSize(vg, h*1.3f);
    nvgFontFace(vg, "icons");
    nvgFillColor(vg, nvgRGBA(255,255,255,96));
    nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
    nvgText(vg, x+w*0.5f-tw*0.5f-iw*0.75f, y+h*0.5f, cpToUTF8(preicon,icon), NULL);
    }

  nvgFontSize(vg, 20.0f);
  nvgFontFace(vg, "sans-bold");
  nvgTextAlign(vg,NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);
  nvgFillColor(vg, nvgRGBA(0,0,0,160));
  nvgText(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f-1,text, NULL);
  nvgFillColor(vg, nvgRGBA(255,255,255,160));
  nvgText(vg, x+w*0.5f-tw*0.5f+iw*0.25f,y+h*0.5f,text, NULL);
  }
//}}}
//{{{
void drawSlider (NVGcontext* vg, float pos, float x, float y, float w, float h) {

  float cy = y+(int)(h*0.5f);
  float kr = (float)(int)(h*0.25f);

  nvgSave(vg);
  //nvgClearState(vg);

  // Slot
  auto bg = nvgBoxGradient(vg, x,cy-2+1, w,4, 2,2, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,128));
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x,cy-2, w,4, 2);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  // Knob Shadow
  bg = nvgRadialGradient(vg, x+(int)(pos*w),cy+1, kr-3,kr+3, nvgRGBA(0,0,0,64), nvgRGBA(0,0,0,0));
  nvgBeginPath(vg);
  nvgRect(vg, x+(int)(pos*w)-kr-5,cy-kr-5,kr*2+5+5,kr*2+5+5+3);
  nvgCircle(vg, x+(int)(pos*w),cy, kr);
  nvgPathWinding(vg, NVG_HOLE);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  // Knob
  auto knob = nvgLinearGradient(vg, x,cy-kr,x,cy+kr, nvgRGBA(255,255,255,16), nvgRGBA(0,0,0,16));
  nvgBeginPath(vg);
  nvgCircle(vg, x+(int)(pos*w),cy, kr-1);
  nvgFillColor(vg, nvgRGBA(40,43,48,255));
  nvgFill(vg);
  nvgFillPaint(vg, knob);
  nvgFill(vg);

  nvgBeginPath(vg);
  nvgCircle(vg, x+(int)(pos*w),cy, kr-0.5f);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,92));
  nvgStroke(vg);

  nvgRestore(vg);
  }
//}}}
//{{{
void drawGraph (NVGcontext* vg, float x, float y, float w, float h, float t) {

  float samples[6];
  float sx[6], sy[6];
  float dx = w/5.0f;
  int i;

  samples[0] = (1+sinf(t*1.2345f+cosf(t*0.33457f)*0.44f))*0.5f;
  samples[1] = (1+sinf(t*0.68363f+cosf(t*1.3f)*1.55f))*0.5f;
  samples[2] = (1+sinf(t*1.1642f+cosf(t*0.33457f)*1.24f))*0.5f;
  samples[3] = (1+sinf(t*0.56345f+cosf(t*1.63f)*0.14f))*0.5f;
  samples[4] = (1+sinf(t*1.6245f+cosf(t*0.254f)*0.3f))*0.5f;
  samples[5] = (1+sinf(t*0.345f+cosf(t*0.03f)*0.6f))*0.5f;

  for (i = 0; i < 6; i++) {
    sx[i] = x+i*dx;
    sy[i] = y+h*samples[i]*0.8f;
    }

  // Graph background
  auto bg = nvgLinearGradient(vg, x,y,x,y+h, nvgRGBA(0,160,192,0), nvgRGBA(0,160,192,64));
  nvgBeginPath(vg);
  nvgMoveTo(vg, sx[0], sy[0]);
  for (i = 1; i < 6; i++)
    nvgBezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
  nvgLineTo(vg, x+w, y+h);
  nvgLineTo(vg, x, y+h);
  nvgFillPaint(vg, bg);
  nvgFill(vg);

  // Graph line
  nvgBeginPath(vg);
  nvgMoveTo(vg, sx[0], sy[0]+2);
  for (i = 1; i < 6; i++)
    nvgBezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1]+2, sx[i]-dx*0.5f,sy[i]+2, sx[i],sy[i]+2);
  nvgStrokeColor(vg, nvgRGBA(0,0,0,32));
  nvgStrokeWidth(vg, 3.0f);
  nvgStroke(vg);

  nvgBeginPath(vg);
  nvgMoveTo(vg, sx[0], sy[0]);
  for (i = 1; i < 6; i++)
    nvgBezierTo(vg, sx[i-1]+dx*0.5f,sy[i-1], sx[i]-dx*0.5f,sy[i], sx[i],sy[i]);
  nvgStrokeColor(vg, nvgRGBA(0,160,192,255));
  nvgStrokeWidth(vg, 3.0f);
  nvgStroke(vg);

  // Graph sample pos
  for (i = 0; i < 6; i++) {
    bg = nvgRadialGradient(vg, sx[i],sy[i]+2, 3.0f,8.0f, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, sx[i]-10, sy[i]-10+2, 20,20);
    nvgFillPaint(vg, bg);
    nvgFill(vg);
    }

  nvgBeginPath(vg);
  for (i = 0; i < 6; i++)
    nvgCircle(vg, sx[i], sy[i], 4.0f);
  nvgFillColor(vg, nvgRGBA(0,160,192,255));
  nvgFill(vg);
  nvgBeginPath(vg);
  for (i = 0; i < 6; i++)
    nvgCircle(vg, sx[i], sy[i], 2.0f);
  nvgFillColor(vg, nvgRGBA(220,220,220,255));
  nvgFill(vg);

  nvgStrokeWidth(vg, 1.0f);
  }
//}}}
//{{{
void drawScissor (NVGcontext* vg, float x, float y, float t) {

  nvgSave(vg);

  // Draw first rect and set scissor to it's area.
  nvgTranslate(vg, x, y);
  nvgRotate(vg, nvgDegToRad(5));
  nvgBeginPath(vg);
  nvgRect(vg, -20,-20,60,40);
  nvgFillColor(vg, nvgRGBA(255,0,0,255));
  nvgFill(vg);
  nvgScissor(vg, -20,-20,60,40);

  // Draw second rectangle with offset and rotation.
  nvgTranslate(vg, 40,0);
  nvgRotate(vg, t);

  // Draw the intended second rectangle without any scissoring.
  nvgSave(vg);
  nvgResetScissor(vg);
  nvgBeginPath(vg);
  nvgRect(vg, -20,-10,60,30);
  nvgFillColor(vg, nvgRGBA(255,128,0,64));
  nvgFill(vg);
  nvgRestore(vg);

  // Draw second rectangle with combined scissoring.
  nvgIntersectScissor(vg, -20,-10,60,30);
  nvgBeginPath(vg);
  nvgRect(vg, -20,-10,60,30);
  nvgFillColor(vg, nvgRGBA(255,128,0,255));
  nvgFill(vg);

  nvgRestore(vg);
  }
//}}}
//{{{
void drawThumbnails (NVGcontext* vg, float x, float y, float w, float h, const int* images, int nimages, float t) {

  float cornerRadius = 3.0f;
  float ix,iy,iw,ih;
  float thumb = 60.0f;
  float arry = 30.5f;
  int imgw, imgh;
  float stackh = (nimages/2) * (thumb+10) + 10;
  int i;
  float u = (1+cosf(t*0.5f))*0.5f;
  float u2 = (1-cosf(t*0.2f))*0.5f;
  float scrollh, dv;

  nvgSave(vg);
  //nvgClearState(vg);

  // Drop shadow
  auto shadowPaint = nvgBoxGradient(vg, x,y+4, w,h, cornerRadius*2, 20, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
  nvgBeginPath(vg);
  nvgRect(vg, x-10,y-10, w+20,h+30);
  nvgRoundedRect(vg, x,y, w,h, cornerRadius);
  nvgPathWinding(vg, NVG_HOLE);
  nvgFillPaint(vg, shadowPaint);
  nvgFill(vg);

  // Window
  nvgBeginPath(vg);
  nvgRoundedRect(vg, x,y, w,h, cornerRadius);
  nvgMoveTo(vg, x-10,y+arry);
  nvgLineTo(vg, x+1,y+arry-11);
  nvgLineTo(vg, x+1,y+arry+11);
  nvgFillColor(vg, nvgRGBA(200,200,200,255));
  nvgFill(vg);

  nvgSave(vg);
  nvgScissor(vg, x,y,w,h);
  nvgTranslate(vg, 0, -(stackh - h)*u);

  dv = 1.0f / (float)(nimages-1);

  for (i = 0; i < nimages; i++) {
    float tx, ty, v, a;
    tx = x+10;
    ty = y+10;
    tx += (i%2) * (thumb+10);
    ty += (i/2) * (thumb+10);
    nvgImageSize(vg, images[i], &imgw, &imgh);
    if (imgw < imgh) {
      iw = thumb;
      ih = iw * (float)imgh/(float)imgw;
      ix = 0;
      iy = -(ih-thumb)*0.5f;
      }
    else {
      ih = thumb;
      iw = ih * (float)imgw/(float)imgh;
      ix = -(iw-thumb)*0.5f;
      iy = 0;
      }

    v = i * dv;
    a = clampf((u2-v) / dv, 0, 1);

    if (a < 1.0f)
      drawSpinner(vg, tx+thumb/2,ty+thumb/2, thumb*0.25f, t);

    auto imgPaint = nvgImagePattern(vg, tx+ix, ty+iy, iw,ih, 0.0f/180.0f*NVG_PI, images[i], a);
    nvgBeginPath(vg);
    nvgRoundedRect(vg, tx,ty, thumb,thumb, 5);
    nvgFillPaint(vg, imgPaint);
    nvgFill(vg);

    auto shadowPaint = nvgBoxGradient(vg, tx-1,ty, thumb+2,thumb+2, 5, 3, nvgRGBA(0,0,0,128), nvgRGBA(0,0,0,0));
    nvgBeginPath(vg);
    nvgRect(vg, tx-5,ty-5, thumb+10,thumb+10);
    nvgRoundedRect(vg, tx,ty, thumb,thumb, 6);
    nvgPathWinding(vg, NVG_HOLE);
    nvgFillPaint(vg, shadowPaint);
    nvgFill(vg);

    nvgBeginPath(vg);
    nvgRoundedRect(vg, tx+0.5f,ty+0.5f, thumb-1,thumb-1, 4-0.5f);
    nvgStrokeWidth(vg,1.0f);
    nvgStrokeColor(vg, nvgRGBA(255,255,255,192));
    nvgStroke(vg);
    }
  nvgRestore(vg);

  // Hide fades
  auto fadePaint = nvgLinearGradient (vg, x,y,x,y+6, nvgRGBA(200,200,200,255), nvgRGBA(200,200,200,0));
  nvgBeginPath (vg);
  nvgRect (vg, x+4,y,w-8,6);
  nvgFillPaint (vg, fadePaint);
  nvgFill (vg);

  fadePaint = nvgLinearGradient (vg, x,y+h,x,y+h-6, nvgRGBA(200,200,200,255), nvgRGBA(200,200,200,0));
  nvgBeginPath (vg);
  nvgRect (vg, x+4,y+h-6,w-8,6);
  nvgFillPaint (vg, fadePaint);
  nvgFill (vg);

  // Scroll bar
  shadowPaint = nvgBoxGradient(vg, x+w-12+1,y+4+1, 8,h-8, 3,4, nvgRGBA(0,0,0,32), nvgRGBA(0,0,0,92));
  nvgBeginPath (vg);
  nvgRoundedRect (vg, x+w-12,y+4, 8,h-8, 3);
  nvgFillPaint (vg, shadowPaint);
  //  nvgFillColor (vg, nvgRGBA(255,0,0,128));
  nvgFill(vg);

  scrollh = (h/stackh) * (h-8);
  shadowPaint = nvgBoxGradient (vg, x+w-12-1,y+4+(h-8-scrollh)*u-1, 8,scrollh, 3,4, nvgRGBA(220,220,220,255), nvgRGBA(128,128,128,255));
  nvgBeginPath (vg);
  nvgRoundedRect (vg, x+w-12+1,y+4+1 + (h-8-scrollh)*u, 8-2,scrollh-2, 2);
  nvgFillPaint (vg, shadowPaint);
  //nvgFillColor(vg, nvgRGBA(0,0,0,128));
  nvgFill (vg);

  nvgRestore (vg);
  }
//}}}
//{{{
void renderDemo (NVGcontext* vg, float cursorX, float cursorY, float width, float height,
                 float t, int blowup, DemoData* data) {

  float x,y,popy;

  drawEyes (vg, width - 250, 50, 150, 100, cursorX, cursorY, t);
  drawParagraph (vg, width - 450, 50, 150, 100, cursorX, cursorY);
  drawGraph (vg, 0, height/2, width, height/2, t);
  drawColorwheel (vg, width - 300, height - 300, 250.0f, 250.0f, t);

  // Line joints
  drawLines(vg, 120, height-50, 600, 50, t);

  // Line caps
  drawWidths(vg, 10, 50, 30);

  // Line caps
  drawCaps(vg, 10, 300, 30);

  drawScissor(vg, 50, height-80, t);

  nvgSave(vg);
  if (blowup) {
    nvgRotate(vg, sinf(t*0.3f)*5.0f/180.0f*NVG_PI);
    nvgScale(vg, 2.0f, 2.0f);
    }

  // Widgets
  drawWindow (vg, "Widgets `n Stuff", 50, 50, 300, 400);
  x = 60; y = 95;
  drawSearchBox(vg, "Search", x,y,280,25);
  y += 40;
  drawDropDown(vg, "Effects", x,y,280,28);
  popy = y + 14;
  y += 45;

  // Form
  drawLabel(vg, "Login", x,y, 280,20);
  y += 25;
  drawEditBox(vg, "Email",  x,y, 280,28);
  y += 35;
  drawEditBox(vg, "Password", x,y, 280,28);
  y += 38;
  drawCheckBox(vg, "Remember me", x,y, 140,28);
  drawButton(vg, ICON_LOGIN, "Sign in", x+138, y, 140, 28, nvgRGBA(0,96,128,255));
  y += 45;

  // Slider
  drawLabel(vg, "Diameter", x,y, 280,20);
  y += 25;
  drawEditBoxNum(vg, "123.00", "px", x+180,y, 100,28);
  drawSlider(vg, 0.4f, x,y, 170,28);
  y += 55;

  drawButton(vg, ICON_TRASH, "Delete", x, y, 160, 28, nvgRGBA(128,16,8,255));
  drawButton(vg, 0, "Cancel", x+170, y, 110, 28, nvgRGBA(0,0,0,0));

  // Thumbnails box
  drawThumbnails(vg, 365, popy-30, 160, 300, data->images, 12, t);

  nvgRestore(vg);
  }
//}}}
