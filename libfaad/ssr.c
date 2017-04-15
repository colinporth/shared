#include "common.h"
#include "structs.h"

#ifdef SSR_DEC

  #include "syntax.h"
  #include "filtbank.h"
  #include "ssr.h"
  #include "ssr_fb.h"

  #ifdef _MSC_VER
    #pragma warning(disable:4305)
    #pragma warning(disable:4244)
  #endif

  /*{{{*/
  static real_t sine_short_32[] = {
      0.0245412290,
      0.0735645667,
      0.1224106774,
      0.1709618866,
      0.2191012502,
      0.2667127550,
      0.3136817515,
      0.3598950505,
      0.4052413106,
      0.4496113360,
      0.4928981960,
      0.5349976420,
      0.5758082271,
      0.6152316332,
      0.6531728506,
      0.6895405650,
      0.7242470980,
      0.7572088838,
      0.7883464694,
      0.8175848126,
      0.8448535800,
      0.8700870275,
      0.8932242990,
      0.9142097831,
      0.9329928160,
      0.9495282173,
      0.9637760520,
      0.9757021666,
      0.9852776527,
      0.9924795628,
      0.9972904325,
      0.9996988177
  };
  /*}}}*/
  /*{{{*/
  static real_t sine_long_256[] = {
      0.0030679568,
      0.0092037553,
      0.0153392069,
      0.0214740802,
      0.0276081469,
      0.0337411724,
      0.0398729295,
      0.0460031852,
      0.0521317050,
      0.0582582653,
      0.0643826351,
      0.0705045760,
      0.0766238645,
      0.0827402696,
      0.0888535529,
      0.0949634984,
      0.1010698676,
      0.1071724296,
      0.1132709533,
      0.1193652153,
      0.1254549921,
      0.1315400302,
      0.1376201212,
      0.1436950415,
      0.1497645378,
      0.1558284014,
      0.1618863940,
      0.1679383069,
      0.1739838719,
      0.1800229102,
      0.1860551536,
      0.1920804083,
      0.1980984211,
      0.2041089684,
      0.2101118416,
      0.2161068022,
      0.2220936269,
      0.2280720919,
      0.2340419590,
      0.2400030345,
      0.2459550500,
      0.2518978119,
      0.2578310966,
      0.2637546957,
      0.2696683407,
      0.2755718231,
      0.2814649343,
      0.2873474658,
      0.2932191789,
      0.2990798354,
      0.3049292266,
      0.3107671738,
      0.3165933788,
      0.3224076927,
      0.3282098472,
      0.3339996636,
      0.3397769034,
      0.3455413282,
      0.3512927592,
      0.3570309579,
      0.3627557456,
      0.3684668541,
      0.3741640747,
      0.3798472285,
      0.3855160773,
      0.3911703825,
      0.3968099952,
      0.4024346471,
      0.4080441594,
      0.4136383235,
      0.4192169011,
      0.4247796834,
      0.4303264916,
      0.4358570874,
      0.4413712919,
      0.4468688369,
      0.4523496032,
      0.4578133225,
      0.4632597864,
      0.4686888456,
      0.4741002321,
      0.4794937670,
      0.4848692715,
      0.4902265072,
      0.4955652654,
      0.5008853674,
      0.5061866641,
      0.5114688873,
      0.5167317986,
      0.5219752789,
      0.5271991491,
      0.5324031115,
      0.5375871062,
      0.5427507758,
      0.5478940606,
      0.5530167222,
      0.5581185222,
      0.5631993413,
      0.5682589412,
      0.5732972026,
      0.5783138275,
      0.5833086967,
      0.5882815719,
      0.5932323337,
      0.5981607437,
      0.6030666232,
      0.6079497933,
      0.6128100753,
      0.6176473498,
      0.6224613190,
      0.6272518039,
      0.6320187449,
      0.6367619038,
      0.6414810419,
      0.6461760402,
      0.6508467197,
      0.6554928422,
      0.6601143479,
      0.6647109985,
      0.6692826152,
      0.6738290191,
      0.6783500314,
      0.6828455329,
      0.6873153448,
      0.6917592883,
      0.6961771250,
      0.7005687952,
      0.7049341202,
      0.7092728615,
      0.7135848999,
      0.7178700566,
      0.7221282125,
      0.7263591886,
      0.7305628061,
      0.7347388864,
      0.7388873696,
      0.7430079579,
      0.7471006513,
      0.7511651516,
      0.7552013993,
      0.7592092156,
      0.7631884217,
      0.7671388984,
      0.7710605264,
      0.7749531269,
      0.7788165212,
      0.7826505899,
      0.7864552140,
      0.7902302146,
      0.7939754725,
      0.7976908684,
      0.8013761640,
      0.8050313592,
      0.8086562157,
      0.8122506142,
      0.8158144355,
      0.8193475604,
      0.8228498101,
      0.8263210654,
      0.8297612667,
      0.8331701756,
      0.8365477324,
      0.8398938179,
      0.8432082534,
      0.8464909792,
      0.8497417569,
      0.8529606462,
      0.8561473489,
      0.8593018055,
      0.8624239564,
      0.8655136228,
      0.8685707450,
      0.8715950847,
      0.8745866418,
      0.8775452971,
      0.8804709315,
      0.8833633661,
      0.8862225413,
      0.8890483975,
      0.8918406963,
      0.8945994973,
      0.8973246217,
      0.9000158906,
      0.9026733041,
      0.9052967429,
      0.9078861475,
      0.9104412794,
      0.9129621983,
      0.9154487252,
      0.9179008007,
      0.9203183055,
      0.9227011204,
      0.9250492454,
      0.9273625612,
      0.9296408892,
      0.9318842888,
      0.9340925813,
      0.9362657070,
      0.9384035468,
      0.9405061007,
      0.9425731897,
      0.9446048737,
      0.9466009140,
      0.9485613704,
      0.9504860640,
      0.9523749948,
      0.9542281032,
      0.9560452700,
      0.9578264356,
      0.9595715404,
      0.9612805247,
      0.9629532695,
      0.9645897746,
      0.9661900401,
      0.9677538276,
      0.9692812562,
      0.9707721472,
      0.9722265005,
      0.9736442566,
      0.9750253558,
      0.9763697386,
      0.9776773453,
      0.9789481759,
      0.9801821709,
      0.9813792109,
      0.9825392962,
      0.9836624265,
      0.9847484827,
      0.9857975245,
      0.9868094325,
      0.9877841473,
      0.9887216687,
      0.9896219969,
      0.9904850721,
      0.9913108945,
      0.9920993447,
      0.9928504229,
      0.9935641289,
      0.9942404628,
      0.9948793054,
      0.9954807758,
      0.9960446954,
      0.9965711236,
      0.9970600605,
      0.9975114465,
      0.9979252815,
      0.9983015656,
      0.9986402392,
      0.9989413023,
      0.9992047548,
      0.9994305968,
      0.9996188283,
      0.9997693896,
      0.9998823404,
      0.9999576211,
      0.9999952912
  };
  /*}}}*/
  /*{{{*/
  static real_t kbd_short_32[] = {
      0.0000875914060105,
      0.0009321760265333,
      0.0032114611466596,
      0.0081009893216786,
      0.0171240286619181,
      0.0320720743527833,
      0.0548307856028528,
      0.0871361822564870,
      0.1302923415174603,
      0.1848955425508276,
      0.2506163195331889,
      0.3260874142923209,
      0.4089316830907141,
      0.4959414909423747,
      0.5833939894958904,
      0.6674601983218376,
      0.7446454751465113,
      0.8121892962974020,
      0.8683559394406505,
      0.9125649996381605,
      0.9453396205809574,
      0.9680864942677585,
      0.9827581789763112,
      0.9914756203467121,
      0.9961964092194694,
      0.9984956609571091,
      0.9994855586984285,
      0.9998533730714648,
      0.9999671864476404,
      0.9999948432453556,
      0.9999995655238333,
      0.9999999961638728
  };

  /*}}}*/
  /*{{{*/
  static real_t kbd_long_256[] = {
      0.0005851230124487,
      0.0009642149851497,
      0.0013558207534965,
      0.0017771849644394,
      0.0022352533849672,
      0.0027342299070304,
      0.0032773001022195,
      0.0038671998069216,
      0.0045064443384152,
      0.0051974336885144,
      0.0059425050016407,
      0.0067439602523141,
      0.0076040812644888,
      0.0085251378135895,
      0.0095093917383048,
      0.0105590986429280,
      0.0116765080854300,
      0.0128638627792770,
      0.0141233971318631,
      0.0154573353235409,
      0.0168678890600951,
      0.0183572550877256,
      0.0199276125319803,
      0.0215811201042484,
      0.0233199132076965,
      0.0251461009666641,
      0.0270617631981826,
      0.0290689473405856,
      0.0311696653515848,
      0.0333658905863535,
      0.0356595546648444,
      0.0380525443366107,
      0.0405466983507029,
      0.0431438043376910,
      0.0458455957104702,
      0.0486537485902075,
      0.0515698787635492,
      0.0545955386770205,
      0.0577322144743916,
      0.0609813230826460,
      0.0643442093520723,
      0.0678221432558827,
      0.0714163171546603,
      0.0751278431308314,
      0.0789577503982528,
      0.0829069827918993,
      0.0869763963425241,
      0.0911667569410503,
      0.0954787380973307,
      0.0999129187977865,
      0.1044697814663005,
      0.1091497100326053,
      0.1139529881122542,
      0.1188797973021148,
      0.1239302155951605,
      0.1291042159181728,
      0.1344016647957880,
      0.1398223211441467,
      0.1453658351972151,
      0.1510317475686540,
      0.1568194884519144,
      0.1627283769610327,
      0.1687576206143887,
      0.1749063149634756,
      0.1811734433685097,
      0.1875578769224857,
      0.1940583745250518,
      0.2006735831073503,
      0.2074020380087318,
      0.2142421635060113,
      0.2211922734956977,
      0.2282505723293797,
      0.2354151558022098,
      0.2426840122941792,
      0.2500550240636293,
      0.2575259686921987,
      0.2650945206801527,
      0.2727582531907993,
      0.2805146399424422,
      0.2883610572460804,
      0.2962947861868143,
      0.3043130149466800,
      0.3124128412663888,
      0.3205912750432127,
      0.3288452410620226,
      0.3371715818562547,
      0.3455670606953511,
      0.3540283646950029,
      0.3625521080463003,
      0.3711348353596863,
      0.3797730251194006,
      0.3884630932439016,
      0.3972013967475546,
      0.4059842374986933,
      0.4148078660689724,
      0.4236684856687616,
      0.4325622561631607,
      0.4414852981630577,
      0.4504336971855032,
      0.4594035078775303,
      0.4683907582974173,
      0.4773914542472655,
      0.4864015836506502,
      0.4954171209689973,
      0.5044340316502417,
      0.5134482766032377,
      0.5224558166913167,
      0.5314526172383208,
      0.5404346525403849,
      0.5493979103766972,
      0.5583383965124314,
      0.5672521391870222,
      0.5761351935809411,
      0.5849836462541291,
      0.5937936195492526,
      0.6025612759529649,
      0.6112828224083939,
      0.6199545145721097,
      0.6285726610088878,
      0.6371336273176413,
      0.6456338401819751,
      0.6540697913388968,
      0.6624380414593221,
      0.6707352239341151,
      0.6789580485595255,
      0.6871033051160131,
      0.6951678668345944,
      0.7031486937449871,
      0.7110428359000029,
      0.7188474364707993,
      0.7265597347077880,
      0.7341770687621900,
      0.7416968783634273,
      0.7491167073477523,
      0.7564342060337386,
      0.7636471334404891,
      0.7707533593446514,
      0.7777508661725849,
      0.7846377507242818,
      0.7914122257259034,
      0.7980726212080798,
      0.8046173857073919,
      0.8110450872887550,
      0.8173544143867162,
      0.8235441764639875,
      0.8296133044858474,
      0.8355608512093652,
      0.8413859912867303,
      0.8470880211822968,
      0.8526663589032990,
      0.8581205435445334,
      0.8634502346476508,
      0.8686552113760616,
      0.8737353715068081,
      0.8786907302411250,
      0.8835214188357692,
      0.8882276830575707,
      0.8928098814640207,
      0.8972684835130879,
      0.9016040675058185,
      0.9058173183656508,
      0.9099090252587376,
      0.9138800790599416,
      0.9177314696695282,
      0.9214642831859411,
      0.9250796989403991,
      0.9285789863994010,
      0.9319635019415643,
      0.9352346855155568,
      0.9383940571861993,
      0.9414432135761304,
      0.9443838242107182,
      0.9472176277741918,
      0.9499464282852282,
      0.9525720912004834,
      0.9550965394547873,
      0.9575217494469370,
      0.9598497469802043,
      0.9620826031668507,
      0.9642224303060783,
      0.9662713777449607,
      0.9682316277319895,
      0.9701053912729269,
      0.9718949039986892,
      0.9736024220549734,
      0.9752302180233160,
      0.9767805768831932,
      0.9782557920246753,
      0.9796581613210076,
      0.9809899832703159,
      0.9822535532154261,
      0.9834511596505429,
      0.9845850806232530,
      0.9856575802399989,
      0.9866709052828243,
      0.9876272819448033,
      0.9885289126911557,
      0.9893779732525968,
      0.9901766097569984,
      0.9909269360049311,
      0.9916310308941294,
      0.9922909359973702,
      0.9929086532976777,
      0.9934861430841844,
      0.9940253220113651,
      0.9945280613237534,
      0.9949961852476154,
      0.9954314695504363,
      0.9958356402684387,
      0.9962103726017252,
      0.9965572899760172,
      0.9968779632693499,
      0.9971739102014799,
      0.9974465948831872,
      0.9976974275220812,
      0.9979277642809907,
      0.9981389072844972,
      0.9983321047686901,
      0.9985085513687731,
      0.9986693885387259,
      0.9988157050968516,
      0.9989485378906924,
      0.9990688725744943,
      0.9991776444921379,
      0.9992757396582338,
      0.9993639958299003,
      0.9994432036616085,
      0.9995141079353859,
      0.9995774088586188,
      0.9996337634216871,
      0.9996837868076957,
      0.9997280538466377,
      0.9997671005064359,
      0.9998014254134544,
      0.9998314913952471,
      0.9998577270385304,
      0.9998805282555989,
      0.9999002598526793,
      0.9999172570940037,
      0.9999318272557038,
      0.9999442511639580,
      0.9999547847121726,
      0.9999636603523446,
      0.9999710885561258,
      0.9999772592414866,
      0.9999823431612708,
      0.9999864932503106,
      0.9999898459281599,
      0.9999925223548691,
      0.9999946296375997,
      0.9999962619864214,
      0.9999975018180320,
      0.9999984208055542,
      0.9999990808746198,
      0.9999995351446231,
      0.9999998288155155
  };
  /*}}}*/

  static real_t **app_pqfbuf;
  static real_t **pp_q0, **pp_t0, **pp_t1;
  /*{{{*/
  static real_t a_half[48] =
  {
      1.2206911375946939E-05,  1.7261986723798209E-05,  1.2300093657077942E-05,
      -1.0833943097791965E-05, -5.7772498639901686E-05, -1.2764767618947719E-04,
      -2.0965186675013334E-04, -2.8166673689263850E-04, -3.1234860429017460E-04,
      -2.6738519958452353E-04, -1.1949424681824722E-04,  1.3965139412648678E-04,
      4.8864136409185725E-04,  8.7044629275148344E-04,  1.1949430269934793E-03,
      1.3519708175026700E-03,  1.2346314373964412E-03,  7.6953209114159191E-04,
      -5.2242432579537141E-05, -1.1516092887213454E-03, -2.3538469841711277E-03,
      -3.4033123072127277E-03, -4.0028551071986133E-03, -3.8745415659693259E-03,
      -2.8321073426874310E-03, -8.5038892323704195E-04,  1.8856751185350931E-03,
      4.9688741735340923E-03,  7.8056704536795926E-03,  9.7027909685901654E-03,
      9.9960423120166159E-03,  8.2019366335594487E-03,  4.1642072876103365E-03,
      -1.8364453822737758E-03, -9.0384863094167686E-03, -1.6241528177129844E-02,
      -2.1939551286300665E-02, -2.4533179947088161E-02, -2.2591663337768787E-02,
      -1.5122066420044672E-02, -1.7971713448186293E-03,  1.6903413428575379E-02,
      3.9672315874127042E-02,  6.4487527248102796E-02,  8.8850025474701726E-02,
      0.1101132906105560    ,  0.1258540205143761    ,  0.1342239368467012
  };
  /*}}}*/

  /*{{{*/
  static void ssr_gain_control (ssr_info *ssr, real_t *data, real_t *output,
                               real_t *overlap, real_t *prev_fmd, uint8_t band,
                               uint8_t window_sequence, uint16_t frame_len)
  {
      uint16_t i;
      real_t gc_function[2*1024/SSR_BANDS];

      if (window_sequence != EIGHT_SHORT_SEQUENCE)
      {
          ssr_gc_function(ssr, &prev_fmd[band * frame_len*2],
              gc_function, window_sequence, band, frame_len);

          for (i = 0; i < frame_len*2; i++)
              data[band * frame_len*2 + i] *= gc_function[i];
          for (i = 0; i < frame_len; i++)
          {
              output[band*frame_len + i] = overlap[band*frame_len + i] +
                  data[band*frame_len*2 + i];
          }
          for (i = 0; i < frame_len; i++)
          {
              overlap[band*frame_len + i] =
                  data[band*frame_len*2 + frame_len + i];
          }
      } else {
          uint8_t w;
          for (w = 0; w < 8; w++)
          {
              uint16_t frame_len8 = frame_len/8;
              uint16_t frame_len16 = frame_len/16;

              ssr_gc_function(ssr, &prev_fmd[band*frame_len*2 + w*frame_len*2/8],
                  gc_function, window_sequence, frame_len);

              for (i = 0; i < frame_len8*2; i++)
                  data[band*frame_len*2 + w*frame_len8*2+i] *= gc_function[i];
              for (i = 0; i < frame_len8; i++)
              {
                  overlap[band*frame_len + i + 7*frame_len16 + w*frame_len8] +=
                      data[band*frame_len*2 + 2*w*frame_len8 + i];
              }
              for (i = 0; i < frame_len8; i++)
              {
                  overlap[band*frame_len + i + 7*frame_len16 + (w+1)*frame_len8] =
                      data[band*frame_len*2 + 2*w*frame_len8 + frame_len8 + i];
              }
          }
          for (i = 0; i < frame_len; i++)
              output[band*frame_len + i] = overlap[band*frame_len + i];
          for (i = 0; i < frame_len; i++)
              overlap[band*frame_len + i] = overlap[band*frame_len + i + frame_len];
      }
  }
  /*}}}*/
  /*{{{*/
  static void ssr_gc_function (ssr_info *ssr, real_t *prev_fmd,
                              real_t *gc_function, uint8_t window_sequence,
                              uint8_t band, uint16_t frame_len)
  {
      uint16_t i;
      uint16_t len_area1, len_area2;
      int32_t aloc[10];
      real_t alev[10];

      switch (window_sequence)
      {
      case ONLY_LONG_SEQUENCE:
          len_area1 = frame_len/SSR_BANDS;
          len_area2 = 0;
          break;
      case LONG_START_SEQUENCE:
          len_area1 = (frame_len/SSR_BANDS)*7/32;
          len_area2 = (frame_len/SSR_BANDS)/16;
          break;
      case EIGHT_SHORT_SEQUENCE:
          len_area1 = (frame_len/8)/SSR_BANDS;
          len_area2 = 0;
          break;
      case LONG_STOP_SEQUENCE:
          len_area1 = (frame_len/SSR_BANDS);
          len_area2 = 0;
          break;
      }

      /* decode bitstream information */

      /* build array M */


      for (i = 0; i < frame_len*2; i++)
          gc_function[i] = 1;
  }
  /*}}}*/
  /*{{{*/
  static INLINE void imdct_ssr(fb_info *fb, real_t *in_data,
                               real_t *out_data, uint16_t len)
  {
      mdct_info *mdct;

      switch (len)
      {
      case 512:
          mdct = fb->mdct2048;
          break;
      case 64:
          mdct = fb->mdct256;
          break;
      }

      faad_imdct(mdct, in_data, out_data);
  }
  /*}}}*/

  /*{{{*/
  void ssr_decode (ssr_info *ssr, fb_info *fb, uint8_t window_sequence,
                  uint8_t window_shape, uint8_t window_shape_prev,
                  real_t *freq_in, real_t *time_out, real_t *overlap,
                  real_t ipqf_buffer[SSR_BANDS][96/4],
                  real_t *prev_fmd, uint16_t frame_len)
  {
      uint8_t band;
      uint16_t ssr_frame_len = frame_len/SSR_BANDS;
      real_t time_tmp[2048] = {0};
      real_t output[1024] = {0};

      for (band = 0; band < SSR_BANDS; band++)
      {
          int16_t j;

          /* uneven bands have inverted frequency scale */
          if (band == 1 || band == 3)
          {
              for (j = 0; j < ssr_frame_len/2; j++)
              {
                  real_t tmp;
                  tmp = freq_in[j + ssr_frame_len*band];
                  freq_in[j + ssr_frame_len*band] =
                      freq_in[ssr_frame_len - j - 1 + ssr_frame_len*band];
                  freq_in[ssr_frame_len - j - 1 + ssr_frame_len*band] = tmp;
              }
          }

          /* non-overlapping inverse filterbank for SSR */
          ssr_ifilter_bank(fb, window_sequence, window_shape, window_shape_prev,
              freq_in + band*ssr_frame_len, time_tmp + band*ssr_frame_len,
              ssr_frame_len);

          /* gain control */
          ssr_gain_control(ssr, time_tmp, output, overlap, prev_fmd,
              band, window_sequence, ssr_frame_len);
      }

      /* inverse pqf to bring subbands together again */
      ssr_ipqf(ssr, output, time_out, ipqf_buffer, frame_len, SSR_BANDS);
  }
  /*}}}*/

  /*{{{*/
  void gc_set_protopqf (real_t *p_proto)
  {
      int j;

      for (j = 0; j < 48; ++j)
      {
          p_proto[j] = p_proto[95-j] = a_half[j];
      }
  }
  /*}}}*/
  /*{{{*/
  void gc_setcoef_eff_pqfsyn (int mm,
                             int kk,
                             real_t *p_proto,
                             real_t ***ppp_q0,
                             real_t ***ppp_t0,
                             real_t ***ppp_t1)
  {
      int i, k, n;
      real_t  w;

      /* Set 1st Mul&Acc Coef's */
      *ppp_q0 = (real_t **) calloc(mm, sizeof(real_t *));
      for (n = 0; n < mm; ++n)
      {
          (*ppp_q0)[n] = (real_t *) calloc(mm, sizeof(real_t));
      }
      for (n = 0; n < mm/2; ++n)
      {
          for (i = 0; i < mm; ++i)
          {
              w = (2*i+1)*(2*n+1-mm)*M_PI/(4*mm);
              (*ppp_q0)[n][i] = 2.0 * cos((real_t) w);

              w = (2*i+1)*(2*(mm+n)+1-mm)*M_PI/(4*mm);
              (*ppp_q0)[n + mm/2][i] = 2.0 * cos((real_t) w);
          }
      }

      /* Set 2nd Mul&Acc Coef's */
      *ppp_t0 = (real_t **) calloc(mm, sizeof(real_t *));
      *ppp_t1 = (real_t **) calloc(mm, sizeof(real_t *));
      for (n = 0; n < mm; ++n)
      {
          (*ppp_t0)[n] = (real_t *) calloc(kk, sizeof(real_t));
          (*ppp_t1)[n] = (real_t *) calloc(kk, sizeof(real_t));
      }
      for (n = 0; n < mm; ++n)
      {
          for (k = 0; k < kk; ++k)
          {
              (*ppp_t0)[n][k] = mm * p_proto[2*k    *mm + n];
              (*ppp_t1)[n][k] = mm * p_proto[(2*k+1)*mm + n];

              if (k%2 != 0)
              {
                  (*ppp_t0)[n][k] = -(*ppp_t0)[n][k];
                  (*ppp_t1)[n][k] = -(*ppp_t1)[n][k];
              }
          }
      }
  }
  /*}}}*/
  /*{{{*/
  void ssr_ipqf (ssr_info *ssr, real_t *in_data, real_t *out_data,
                real_t buffer[SSR_BANDS][96/4],
                uint16_t frame_len, uint8_t bands)
  {
      static int initFlag = 0;
      real_t a_pqfproto[PQFTAPS];

      int i;

      if (initFlag == 0)
      {
          gc_set_protopqf(a_pqfproto);
          gc_setcoef_eff_pqfsyn(SSR_BANDS, PQFTAPS/(2*SSR_BANDS), a_pqfproto,
              &pp_q0, &pp_t0, &pp_t1);
          initFlag = 1;
      }

      for (i = 0; i < frame_len / SSR_BANDS; i++)
      {
          int l, n, k;
          int mm = SSR_BANDS;
          int kk = PQFTAPS/(2*SSR_BANDS);

          for (n = 0; n < mm; n++)
          {
              for (k = 0; k < 2*kk-1; k++)
              {
                  buffer[n][k] = buffer[n][k+1];
              }
          }

          for (n = 0; n < mm; n++)
          {
              real_t acc = 0.0;
              for (l = 0; l < mm; l++)
              {
                  acc += pp_q0[n][l] * in_data[l*frame_len/SSR_BANDS + i];
              }
              buffer[n][2*kk-1] = acc;
          }

          for (n = 0; n < mm/2; n++)
          {
              real_t acc = 0.0;
              for (k = 0; k < kk; k++)
              {
                  acc += pp_t0[n][k] * buffer[n][2*kk-1-2*k];
              }
              for (k = 0; k < kk; ++k)
              {
                  acc += pp_t1[n][k] * buffer[n + mm/2][2*kk-2-2*k];
              }
              out_data[i*SSR_BANDS + n] = acc;

              acc = 0.0;
              for (k = 0; k < kk; k++)
              {
                  acc += pp_t0[mm-1-n][k] * buffer[n][2*kk-1-2*k];
              }
              for (k = 0; k < kk; k++)
              {
                  acc -= pp_t1[mm-1-n][k] * buffer[n + mm/2][2*kk-2-2*k];
              }
              out_data[i*SSR_BANDS + mm-1-n] = acc;
          }
      }
  }
  /*}}}*/

  /*{{{*/
  fb_info *ssr_filter_bank_init (uint16_t frame_len)
  {
      uint16_t nshort = frame_len/8;

      fb_info *fb = (fb_info*)faad_malloc(sizeof(fb_info));
      memset(fb, 0, sizeof(fb_info));

      /* normal */
      fb->mdct256 = faad_mdct_init(2*nshort);
      fb->mdct2048 = faad_mdct_init(2*frame_len);

      fb->long_window[0]  = sine_long_256;
      fb->short_window[0] = sine_short_32;
      fb->long_window[1]  = kbd_long_256;
      fb->short_window[1] = kbd_short_32;

      return fb;
  }
  /*}}}*/
  /*{{{*/
  void ssr_filter_bank_end (fb_info* fb) {

    faad_mdct_end (fb->mdct256);
    faad_mdct_end (fb->mdct2048);
    faad_free(fb);
    }
  /*}}}*/

  /*{{{*/
  /* NON-overlapping inverse filterbank for use with SSR */
  void ssr_ifilter_bank (fb_info* fb, uint8_t window_sequence, uint8_t window_shape,
                         uint8_t window_shape_prev, real_t* freq_in, real_t* time_out, uint16_t frame_len)
  {
      int16_t i;
      real_t *transf_buf;

      real_t *window_long;
      real_t *window_long_prev;
      real_t *window_short;
      real_t *window_short_prev;

      uint16_t nlong = frame_len;
      uint16_t nshort = frame_len/8;
      uint16_t trans = nshort/2;

      uint16_t nflat_ls = (nlong-nshort)/2;

      transf_buf = (real_t*)faad_malloc(2*nlong*sizeof(real_t));

      window_long       = fb->long_window[window_shape];
      window_long_prev  = fb->long_window[window_shape_prev];
      window_short      = fb->short_window[window_shape];
      window_short_prev = fb->short_window[window_shape_prev];

      switch (window_sequence)
      {
      case ONLY_LONG_SEQUENCE:
          imdct_ssr(fb, freq_in, transf_buf, 2*nlong);
          for (i = nlong-1; i >= 0; i--)
          {
              time_out[i] = MUL_R_C(transf_buf[i],window_long_prev[i]);
              time_out[nlong+i] = MUL_R_C(transf_buf[nlong+i],window_long[nlong-1-i]);
          }
          break;

      case LONG_START_SEQUENCE:
          imdct_ssr(fb, freq_in, transf_buf, 2*nlong);
          for (i = 0; i < nlong; i++)
              time_out[i] = MUL_R_C(transf_buf[i],window_long_prev[i]);
          for (i = 0; i < nflat_ls; i++)
              time_out[nlong+i] = transf_buf[nlong+i];
          for (i = 0; i < nshort; i++)
              time_out[nlong+nflat_ls+i] = MUL_R_C(transf_buf[nlong+nflat_ls+i],window_short[nshort-i-1]);
          for (i = 0; i < nflat_ls; i++)
              time_out[nlong+nflat_ls+nshort+i] = 0;
          break;

      case EIGHT_SHORT_SEQUENCE:
          imdct_ssr(fb, freq_in+0*nshort, transf_buf+2*nshort*0, 2*nshort);
          imdct_ssr(fb, freq_in+1*nshort, transf_buf+2*nshort*1, 2*nshort);
          imdct_ssr(fb, freq_in+2*nshort, transf_buf+2*nshort*2, 2*nshort);
          imdct_ssr(fb, freq_in+3*nshort, transf_buf+2*nshort*3, 2*nshort);
          imdct_ssr(fb, freq_in+4*nshort, transf_buf+2*nshort*4, 2*nshort);
          imdct_ssr(fb, freq_in+5*nshort, transf_buf+2*nshort*5, 2*nshort);
          imdct_ssr(fb, freq_in+6*nshort, transf_buf+2*nshort*6, 2*nshort);
          imdct_ssr(fb, freq_in+7*nshort, transf_buf+2*nshort*7, 2*nshort);
          for(i = nshort-1; i >= 0; i--)
          {
              time_out[i+0*nshort] = MUL_R_C(transf_buf[nshort*0+i],window_short_prev[i]);
              time_out[i+1*nshort] = MUL_R_C(transf_buf[nshort*1+i],window_short[i]);
              time_out[i+2*nshort] = MUL_R_C(transf_buf[nshort*2+i],window_short_prev[i]);
              time_out[i+3*nshort] = MUL_R_C(transf_buf[nshort*3+i],window_short[i]);
              time_out[i+4*nshort] = MUL_R_C(transf_buf[nshort*4+i],window_short_prev[i]);
              time_out[i+5*nshort] = MUL_R_C(transf_buf[nshort*5+i],window_short[i]);
              time_out[i+6*nshort] = MUL_R_C(transf_buf[nshort*6+i],window_short_prev[i]);
              time_out[i+7*nshort] = MUL_R_C(transf_buf[nshort*7+i],window_short[i]);
              time_out[i+8*nshort] = MUL_R_C(transf_buf[nshort*8+i],window_short_prev[i]);
              time_out[i+9*nshort] = MUL_R_C(transf_buf[nshort*9+i],window_short[i]);
              time_out[i+10*nshort] = MUL_R_C(transf_buf[nshort*10+i],window_short_prev[i]);
              time_out[i+11*nshort] = MUL_R_C(transf_buf[nshort*11+i],window_short[i]);
              time_out[i+12*nshort] = MUL_R_C(transf_buf[nshort*12+i],window_short_prev[i]);
              time_out[i+13*nshort] = MUL_R_C(transf_buf[nshort*13+i],window_short[i]);
              time_out[i+14*nshort] = MUL_R_C(transf_buf[nshort*14+i],window_short_prev[i]);
              time_out[i+15*nshort] = MUL_R_C(transf_buf[nshort*15+i],window_short[i]);
          }
          break;

      case LONG_STOP_SEQUENCE:
          imdct_ssr(fb, freq_in, transf_buf, 2*nlong);
          for (i = 0; i < nflat_ls; i++)
              time_out[i] = 0;
          for (i = 0; i < nshort; i++)
              time_out[nflat_ls+i] = MUL_R_C(transf_buf[nflat_ls+i],window_short_prev[i]);
          for (i = 0; i < nflat_ls; i++)
              time_out[nflat_ls+nshort+i] = transf_buf[nflat_ls+nshort+i];
          for (i = 0; i < nlong; i++)
              time_out[nlong+i] = MUL_R_C(transf_buf[nlong+i],window_long[nlong-1-i]);
      break;
      }

      faad_free(transf_buf);
  }

  /*}}}*/
#endif
