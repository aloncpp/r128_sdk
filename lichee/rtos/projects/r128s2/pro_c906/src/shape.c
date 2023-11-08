#include "shape.h"
#include <stdlib.h>


/*
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
*/


/*
        * * * * 
      *   *     * 
    *     *       *
    *     *       *
    *     * * *   *
    *             *
      *         * 
        * * * * 
*/
static uint32_t shape_clock[8][8] =
{
    {0,0,1,1,1,1,0,0},
    {0,1,2,1,2,2,1,0},
    {1,2,2,1,2,2,2,1},
    {1,2,2,1,2,2,2,1},
    {1,2,2,1,1,1,2,1},
    {1,2,2,2,2,2,2,1},
    {0,1,2,2,2,2,1,0},
    {0,0,1,1,1,1,0,0},
};

/*
        * * * * 
      *   *     * 
    *     *       *
    *     *       *
    *     * * *   *
    *             *
      *         * 
        * * * * 
*/
static uint32_t shape_degree[8][8] =
{
    {1,1,1,0,0,0,0,0},
    {1,1,1,0,0,0,0,0},
    {1,1,1,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,0,0,0,0,0},
};

/*
         * * *
       *       *
     *     *     *
         *   *
           *
*/
static uint32_t shape_wifi[8][8] =
{
    {0,0,0,0,0,0,0,0},
    {0,0,1,1,1,0,0,0},
    {0,1,0,0,0,1,0,0},
    {1,0,0,1,0,0,1,0},
    {0,0,1,0,1,0,0,0},
    {0,0,0,0,0,0,0,0},
    {0,0,0,1,0,0,0,0},
    {0,0,0,0,0,0,0,0},
};
/*
           * 
         *   *
         *   *
         *   *
         *   *
       *       *
       *       *
         * * *
*/
static uint32_t shape_temp[8][8] = 
{
    {0,0,0,1,0,0,0,0},
    {0,0,1,2,1,0,0,0},
    {0,0,1,2,1,0,0,0},
    {0,0,1,2,1,0,0,0},
    {0,0,1,2,1,0,0,0},
    {0,1,2,2,2,1,0,0},
    {0,1,2,2,2,1,0,0},
    {0,0,1,1,1,0,0,0},
};

static uint32_t shape_sunny[8][8] = 
{
    {                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},
    {SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW},
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,                 0},
    {SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW},
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},
    {                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0},

};

static uint32_t shape_cloudy[8][8] = 
{
    {                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},
    {SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_YELLOW},
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0},
    {                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_YELLOW,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_YELLOW,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0},
};

static uint32_t shape_rain[8][8] = 
{
    {                 0,                 0,                 0,                 0,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0},
    {                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE},
    {                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,                 0},
    {                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D},
    {SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0},
    {                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0},

};

static uint32_t shape_snow[8][8] = 
{
    {SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0,SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_BLUE_D},
    {                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0},
    {SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_L},
    {                 0,                 0,                 0,SHAPE_COLOR_BLUE_D,SHAPE_COLOR_BLUE_L,                 0,                 0,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_BLUE_L,SHAPE_COLOR_BLUE_D,                 0,                 0,                 0},
    {SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_L},
    {                 0,SHAPE_COLOR_BLUE_D,                 0,                 0,                 0,                 0,SHAPE_COLOR_BLUE_L,                 0},
    {SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0,SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_L},

};

static uint32_t shape_fog[8][8] = 
{
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  },
    {SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0},
    {                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  },
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,                 0},
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,                 0},

};

static uint32_t shape_windy[8][8] = 
{
    {                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,                 0,                 0,                 0,                 0},
    {SHAPE_COLOR_WHIITE,                 0,                 0,SHAPE_COLOR_WHIITE,                 0,                 0,                 0,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,                 0},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_GREP  ,                 0,                 0,SHAPE_COLOR_GREP  },
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,SHAPE_COLOR_GREP  },
    {SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,                 0},
    {                 0,                 0,                 0,                 0,                 0,                 0,                 0,SHAPE_COLOR_GREP  },
    {SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,SHAPE_COLOR_GREP  ,                 0},

};

static uint32_t shape_cold[8][8] = 
{
    {                 0,                 0,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,                 0,                 0},
    {                 0,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_BLUE_D,SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_WHIITE,                 0},
    {                 0,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_D,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_BLUE_L,SHAPE_COLOR_WHIITE},
    {SHAPE_COLOR_WHIITE,SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_BLUE_D,                 0,SHAPE_COLOR_BLUE_D,SHAPE_COLOR_WHIITE},
    {                 0,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0,SHAPE_COLOR_BLUE_L,                 0,                 0},
    {                 0,SHAPE_COLOR_WHIITE,                 0,SHAPE_COLOR_BLUE_D,SHAPE_COLOR_BLUE_L,                 0,SHAPE_COLOR_WHIITE,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_WHIITE,SHAPE_COLOR_WHIITE,                 0,                 0,                 0},
};

static uint32_t shape_hot[8][8] = 
{
    {                 0,                 0,                 0,SHAPE_COLOR_YELLOW,                 0,                 0,                 0,                 0},
    {                 0,                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_YELLOW,                 0,                 0,                 0},
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_RED   ,SHAPE_COLOR_YELLOW,                 0,                 0,                 0},
    {                 0,                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_ORANGE,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_YELLOW,                 0,                 0},
    {                 0,SHAPE_COLOR_YELLOW,SHAPE_COLOR_ORANGE,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_YELLOW,                 0},
    {SHAPE_COLOR_YELLOW,SHAPE_COLOR_ORANGE,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_RED   ,SHAPE_COLOR_YELLOW,                 0},
    {SHAPE_COLOR_YELLOW,SHAPE_COLOR_ORANGE,SHAPE_COLOR_ORANGE,SHAPE_COLOR_ORANGE,SHAPE_COLOR_ORANGE,SHAPE_COLOR_ORANGE,SHAPE_COLOR_ORANGE,SHAPE_COLOR_YELLOW},
};

static uint32_t shape_rabb[6][5] =
{
    {0,3,0,3,0},
    {2,3,2,3,2},
    {2,2,2,2,2},
    {1,1,1,1,1},
    {1,0,1,0,1},
    {1,1,1,1,1},
};

int weather_shape_init(weather_shape_t *shape)
{
  shape[0].shape = (uint32_t *)&shape_sunny;
  shape[1].shape = (uint32_t *)&shape_cloudy;
  shape[2].shape = (uint32_t *)&shape_rain;
  shape[3].shape = (uint32_t *)&shape_snow;
  shape[4].shape = (uint32_t *)&shape_fog;
  shape[5].shape = (uint32_t *)&shape_windy;
  shape[6].shape = (uint32_t *)&shape_cold;
  shape[7].shape = (uint32_t *)&shape_hot;

  return 0;
}

int shape_8x8_init(icon_shape_t *shape)
{
  shape[0].shape = (uint32_t *)&shape_clock;
  shape[1].shape = (uint32_t *)&shape_wifi;
  shape[2].shape = (uint32_t *)&shape_temp;
  shape[3].shape = (uint32_t *)&shape_degree;

  return 0;
}

int shape_5x6_init(icon_shape_t *shape)
{
    shape[0].shape = (uint32_t *)&shape_rabb;

    return 0;
}
