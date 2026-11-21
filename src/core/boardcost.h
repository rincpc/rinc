/* boardcost.h: what each thing costs on the Fruit Jam, for board speed (rinc.c, plans/board-speed.md).
   Made by tools/boardspeed.py from the board's own measurements (2026-11-20): not edited by hand.
   An op's group (rinc_opGroup, by op: +256 in code the board's JIT compiles) costs rinc_groupNs; a call
   of each kind (rinc_COST_KINDS) costs rinc_kindNs: once, and for each pixel or byte. */
/* costs (ns): move=50.075 movn=270.873 int=72.774 mul=68.020 div=118.237 float=91.309 fdiv=134.419 math=402.819 load=0.000 store=33.635 index=135.340 vector=0.000 jump=76.091 loop=141.070 call=104.244 calli=664.224 ffi=178.810 list=272.021 bytes=137.318 strcmp=1206.031 log=0.000 other=0.000 n_alu=5.104 n_div=45.587 n_float=20.144 n_fdiv=74.280 n_math=403.945 n_mem=68.412 n_jump=9.670 n_loop=31.613 n_call=123.382 n_ffi=148.034 n_other=0.000 cls.call=0.000 cls.each=2.325 rect.call=0.000 rect.each=2.963 rectRows.call=47.942 rectRows.each=171.054 pset.call=5.351 pset.each=0.000 pget.call=14.324 pget.each=0.000 blit.call=3141.808 blit.each=35.179 blitPacked.call=0.000 blitPacked.each=240.009 blitScaled.call=3761.673 blitScaled.each=90.292 blitBlocks.call=195.623 blitBlocks.each=43.817 print.call=3228.728 print.each=3775.107 printFont.call=0.000 printFont.each=0.000 textSize.call=0.000 textSize.each=314.683 fileRead.call=2645122.723 fileRead.each=754.272 fileWrite.call=19987151.804 fileWrite.each=2182.419 dir.call=0.000 dir.each=522670.434 imageLoad.call=0.000 imageLoad.each=2555.670 imageWrite.call=0.000 imageWrite.each=8280.330 */
#define rinc_BOARD_MEASURED "2026-11-20"
static const uint8_t rinc_opGroup[512] = {
  0, 2, 2, 3, 4, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 5, 6, 5, 2, 2, 2, 2, 5, 5, 5, 5, 19, 19, 5, 5,
  8, 8, 8, 7, 7, 2, 2, 2, 5, 5, 10, 10, 0, 8, 9, 9, 9, 10, 10, 10, 18, 1, 1, 1, 18, 18, 11, 11, 11, 11, 11, 11,
  11, 11, 11, 10, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 10, 17, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 13,
  13, 13, 14, 14, 15, 14, 16, 20, 20, 20, 20, 20, 20, 20, 21, 21, 21, 10, 18, 18, 19, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  0, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 5, 5, 5, 8, 8, 8, 8, 9, 9, 9, 1, 0, 12, 12, 12, 12, 12, 12,
  12, 13, 13, 14, 14, 16, 21, 0, 0, 0, 0, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 2, 5, 5, 5, 5, 5, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  22, 22, 22, 22, 23, 23, 22, 22, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 25, 24, 22, 22, 22, 22, 24, 24, 24, 24, 32, 32, 24, 24,
  22, 22, 22, 26, 26, 22, 22, 22, 24, 24, 27, 27, 22, 22, 22, 22, 22, 27, 27, 27, 32, 32, 32, 32, 32, 32, 22, 22, 22, 22, 22, 22,
  22, 22, 22, 27, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 27, 32, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 28, 29,
  29, 29, 30, 30, 30, 30, 31, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 27, 32, 32, 32, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24, 24, 22, 22, 22, 22, 22, 22, 22, 32, 22, 28, 28, 28, 28, 28, 28,
  28, 29, 29, 30, 30, 31, 32, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 22, 24, 24, 24, 24, 24, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
  21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21, 21,
};
static const float rinc_groupNs[33] = {
  50.075f, /* move */
  270.873f, /* movn */
  72.774f, /* int */
  68.020f, /* mul */
  118.237f, /* div */
  91.309f, /* float */
  134.419f, /* fdiv */
  402.819f, /* math */
  0.000f, /* load */
  33.635f, /* store */
  135.340f, /* index */
  0.000f, /* vector */
  76.091f, /* jump */
  141.070f, /* loop */
  104.244f, /* call */
  664.224f, /* calli */
  178.810f, /* ffi */
  272.021f, /* list */
  137.318f, /* bytes */
  1206.031f, /* strcmp */
  0.000f, /* log */
  0.000f, /* other */
  5.104f, /* n_alu */
  45.587f, /* n_div */
  20.144f, /* n_float */
  74.280f, /* n_fdiv */
  403.945f, /* n_math */
  68.412f, /* n_mem */
  9.670f, /* n_jump */
  31.613f, /* n_loop */
  123.382f, /* n_call */
  148.034f, /* n_ffi */
  0.000f, /* n_other */
};
static const float rinc_kindNs[rinc_Cost_count][2] = {
  [rinc_Cost_cls] = {0.000f, 2.3251f},
  [rinc_Cost_rect] = {0.000f, 2.9627f},
  [rinc_Cost_rectRows] = {47.942f, 171.0539f},
  [rinc_Cost_pset] = {5.351f, 0.0000f},
  [rinc_Cost_pget] = {14.324f, 0.0000f},
  [rinc_Cost_blit] = {3141.808f, 35.1791f},
  [rinc_Cost_blitPacked] = {0.000f, 240.0092f},
  [rinc_Cost_blitScaled] = {3761.673f, 90.2921f},
  [rinc_Cost_blitBlocks] = {195.623f, 43.8171f},
  [rinc_Cost_print] = {3228.728f, 3775.1072f},
  [rinc_Cost_printFont] = {0.000f, 0.0000f},
  [rinc_Cost_textSize] = {0.000f, 314.6835f},
  [rinc_Cost_fileRead] = {2645122.723f, 754.2723f},
  [rinc_Cost_fileWrite] = {19987151.804f, 2182.4186f},
  [rinc_Cost_dir] = {0.000f, 522670.4336f},
  [rinc_Cost_imageLoad] = {0.000f, 2555.6704f},
  [rinc_Cost_imageWrite] = {0.000f, 8280.3300f},
};
