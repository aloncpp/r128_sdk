#include "fonts.h"



/*
    (space)
*/
static const uint8_t font_ascii_32[5][3] =
{
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};
/*
    ！
*/
static const uint8_t font_ascii_33[5][3] =
{
    {0, 1, 0},
    {0, 1, 0},
    {0, 1, 0},
    {0, 0, 0},
    {0, 1, 0},
};

/*
    "
*/
static const uint8_t font_ascii_34[5][3] =
{
    {1, 0, 1},
    {1, 0, 1},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

/*
    #
*/
static const uint8_t font_ascii_35[5][3] =
{
    {1, 0, 1},
    {1, 1, 1},
    {1, 0, 1},
    {1, 1, 1},
    {1, 0, 1},
};

/*
    $
*/
static const uint8_t font_ascii_36[5][3] =
{
    {0, 1, 0},
    {1, 1, 1},
    {0, 1, 0},
    {1, 1, 1},
    {0, 1, 0},
};

/*
    %
*/
static const uint8_t font_ascii_37[5][3] =
{
    {1, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, 1},
};

/*
    &
*/
static const uint8_t font_ascii_38[5][3] =
{
    {0, 1, 0},
    {0, 0, 1},
    {1, 1, 0},
    {1, 0, 1},
    {0, 1, 0},
};

/*
    '
*/
static const uint8_t font_ascii_39[5][3] =
{

    // {0, 1, 0},
    // {0, 1, 0},
    // {0, 0, 0},
    // {0, 0, 0},
    // {0, 0, 0},
    
    {1, 1, 0},
    {1, 1, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
};

/*
    (
*/
static const uint8_t font_ascii_40[5][3] =
{
    {0, 1, 0},
    {1, 0, 0},
    {1, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
};

/*
    )
*/
static const uint8_t font_ascii_41[5][3] =
{
    {0, 1, 0},
    {0, 0, 1},
    {0, 0, 1},
    {0, 0, 1},
    {0, 1, 0},
};

/*
    *
*/
static const uint8_t font_ascii_42[5][3] =
{
    {0, 0, 0},
    {0, 1, 0},
    {1, 0, 1},
    {0, 1, 0},
    {0, 0, 0},
};

/*
    +
*/
static const uint8_t font_ascii_43[5][3] =
{
    {0, 0, 0},
    {0, 1, 0},
    {1, 1, 1},
    {0, 1, 0},
    {0, 0, 0},
};
/*
    ,
*/
static const uint8_t font_ascii_44[5][3] =
{
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {0, 1, 0},
};
/*
    -
*/
static const uint8_t font_ascii_45[5][3] =
{
    {0, 0, 0},
    {0, 0, 0},
    {1, 1, 1},
    {0, 0, 0},
    {0, 0, 0},
};
/*
    .
*/
static const uint8_t font_ascii_46[5][3] =
{
    {0, 0, 0},
    {0, 0, 0},
    {0, 0, 0},
    {0, 1, 0},
    {0, 0, 0},
};
/*
    /
*/
static const uint8_t font_ascii_47[5][3] =
{
    {0, 0, 0},
    {0, 0, 1},
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, 0},
};
/*
    0
*/
static const uint8_t font_ascii_48[5][3] =
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    1
*/
static const uint8_t font_ascii_49[5][3] =
    {
        {0, 1, 0},
        {1, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
        {1, 1, 1},
};
/*
    2
*/
static const uint8_t font_ascii_50[5][3] =
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
};
/*
    3
*/
static const uint8_t font_ascii_51[5][3] =
    {
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
};
/*
    4
*/
static const uint8_t font_ascii_52[5][3] =
    {
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
};
/*
    5
*/
static const uint8_t font_ascii_53[5][3] =
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
};
/*
    6
*/
static const uint8_t font_ascii_54[5][3] =
    {
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    7
*/
static const uint8_t font_ascii_55[5][3] =
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
};
/*
    8
*/
static const uint8_t font_ascii_56[5][3] =
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    9
*/
static const uint8_t font_ascii_57[5][3] =
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {0, 0, 1},
        {1, 1, 1},
};
/*
    :
*/
static const uint8_t font_ascii_58[5][3] =
    {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 0},
};
/*
    ;
*/
static const uint8_t font_ascii_59[5][3] =
    {
        {0, 0, 0},
        {0, 1, 0},
        {0, 0, 0},
        {0, 1, 0},
        {0, 1, 0},
};
/*
    <
*/
static const uint8_t font_ascii_60[5][3] =
    {
        {0, 0, 1},
        {0, 1, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
};
/*
    =
*/
static const uint8_t font_ascii_61[5][3] =
    {
        {0, 0, 0},
        {1, 1, 1},
        {0, 0, 0},
        {1, 1, 1},
        {0, 0, 0},
};
/*
    >
*/
static const uint8_t font_ascii_62[5][3] =
    {
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {0, 1, 0},
        {1, 0, 0},
};
/*
    ？
*/
static const uint8_t font_ascii_63[5][3] =
    {
        {1, 1, 1},
        {0, 0, 1},
        {0, 1, 1},
        {0, 0, 0},
        {0, 1, 0},
};

/*
    @
*/
static const uint8_t font_ascii_64[5][3] =
    {
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 0},
        {1, 1, 1},
};

/*
    A
*/
static const uint8_t font_ascii_65[5][3] =
{
        {0, 1, 0},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
};

/*
    B
*/
static const uint8_t font_ascii_66[5][3] =
{
        {1, 1, 0},
        {1, 0, 1},
        {1, 1, 0},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    C
*/
static const uint8_t font_ascii_67[5][3] =
{
        {1, 1, 1},
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 1, 1},
};
/*
    D
*/
static const uint8_t font_ascii_68[5][3] =
{
        {1, 1, 0},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 0},
};
/*
    E
*/
static const uint8_t font_ascii_69[5][3] =
{
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
};
/*
    F
*/
static const uint8_t font_ascii_70[5][3] =
{
        {1, 1, 1},
        {1, 0, 0},
        {1, 1, 1},
        {1, 0, 0},
        {1, 0, 0},
};
/*
    G
*/
static const uint8_t font_ascii_71[5][3] =
{
        {1, 1, 1},
        {1, 0, 0},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    H
*/
static const uint8_t font_ascii_72[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
};
/*
    I
*/
static const uint8_t font_ascii_73[5][3] =
{
        {1, 1, 1},
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
        {1, 1, 1},
};
/*
    G
*/
static const uint8_t font_ascii_74[5][3] =
{
        {1, 1, 1},
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
        {1, 1, 0},
};
/*
    K
*/
static const uint8_t font_ascii_75[5][3] =
{
        {1, 0, 1},
        {1, 1, 0},
        {1, 0, 0},
        {1, 1, 0},
        {1, 0, 1},
};
/*
    L
*/
static const uint8_t font_ascii_76[5][3] =
{
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 1, 1},
};
/*
    M
*/
static const uint8_t font_ascii_77[5][3] =
{
        {1, 0, 1},
        {1, 1, 1},
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
};
/*
    N
*/
static const uint8_t font_ascii_78[5][3] =
{
        {1, 1, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
};
/*
    O
*/
static const uint8_t font_ascii_79[5][3] =
{
        {0, 1, 0},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {0, 1, 0},
};
/*
    P
*/
static const uint8_t font_ascii_80[5][3] =
{
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 0},
        {1, 0, 0},
};
/*
    Q
*/
static const uint8_t font_ascii_81[5][3] =
{
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 0, 1},
        {0, 0, 1},
};
/*
    R
*/
static const uint8_t font_ascii_82[5][3] =
{
        {1, 1, 1},
        {1, 0, 1},
        {1, 1, 0},
        {1, 0, 1},
        {1, 0, 1},
};
/*
    S
*/
static const uint8_t font_ascii_83[5][3] =
{
        {1, 1, 1},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {1, 1, 1},
};
/*
    T
*/
static const uint8_t font_ascii_84[5][3] =
{
        {1, 1, 1},
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
        {0, 1, 0},
};
/*
    U
*/
static const uint8_t font_ascii_85[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
};
/*
    V
*/
static const uint8_t font_ascii_86[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {0, 1, 0},
};
/*
    W
*/
static const uint8_t font_ascii_87[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {1, 1, 1},
        {1, 1, 1},
        {1, 0, 1},
};
/*
    X
*/
static const uint8_t font_ascii_88[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {0, 1, 0},
        {1, 0, 1},
        {1, 0, 1},
};
/*
    Y
*/
static const uint8_t font_ascii_89[5][3] =
{
        {1, 0, 1},
        {1, 0, 1},
        {1, 0, 1},
        {0, 1, 0},
        {0, 1, 0},
};
/*
    Z
*/
static const uint8_t font_ascii_90[5][3] =
{
        {1, 1, 1},
        {0, 0, 1},
        {0, 1, 0},
        {1, 0, 0},
        {1, 1, 1},
};
/*
    [
*/
static const uint8_t font_ascii_91[5][3] =
{
        {1, 1, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 0, 0},
        {1, 1, 0},
};
/*
    \
*/
static const uint8_t font_ascii_92[5][3] =
{
        {0, 0, 0},
        {1, 0, 0},
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0},
};
/*
    ]
*/
static const uint8_t font_ascii_93[5][3] =
{
        {0, 1, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 0, 1},
        {0, 1, 1},
};
/*
    ^
*/
static const uint8_t font_ascii_94[5][3] =
{
        {0, 1, 0},
        {1, 0, 1},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
};
/*
    _
*/
static const uint8_t font_ascii_95[5][3] =
{
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
        {1, 1, 1},
};
/*
    `
*/
static const uint8_t font_ascii_96[5][3] =
{
        {0, 1, 0},
        {0, 0, 1},
        {0, 0, 0},
        {0, 0, 0},
        {0, 0, 0},
};



uint8_t fonts_ascii_5_3_init(ascii_5_3_font_t *font)
{
    font[0].font  = (uint8_t *)&font_ascii_32;
    font[1].font  = (uint8_t *)&font_ascii_33;
    font[2].font  = (uint8_t *)&font_ascii_34;
    font[3].font  = (uint8_t *)&font_ascii_35;
    font[4].font  = (uint8_t *)&font_ascii_36;
    font[5].font  = (uint8_t *)&font_ascii_37;
    font[6].font  = (uint8_t *)&font_ascii_38;
    font[7].font  = (uint8_t *)&font_ascii_39;
    font[8].font  = (uint8_t *)&font_ascii_40;
    font[9].font  = (uint8_t *)&font_ascii_41;
    font[10].font = (uint8_t *)&font_ascii_42;
    font[11].font = (uint8_t *)&font_ascii_43;
    font[12].font = (uint8_t *)&font_ascii_44;
    font[13].font = (uint8_t *)&font_ascii_45;
    font[14].font = (uint8_t *)&font_ascii_46;
    font[15].font = (uint8_t *)&font_ascii_47;
    font[16].font = (uint8_t *)&font_ascii_48;
    font[17].font = (uint8_t *)&font_ascii_49;
    font[18].font = (uint8_t *)&font_ascii_50;
    font[19].font = (uint8_t *)&font_ascii_51;
    font[20].font = (uint8_t *)&font_ascii_52;
    font[21].font = (uint8_t *)&font_ascii_53;
    font[22].font = (uint8_t *)&font_ascii_54;
    font[23].font = (uint8_t *)&font_ascii_55;
    font[24].font = (uint8_t *)&font_ascii_56;
    font[25].font = (uint8_t *)&font_ascii_57;
    font[26].font = (uint8_t *)&font_ascii_58;
    font[27].font = (uint8_t *)&font_ascii_59;
    font[28].font = (uint8_t *)&font_ascii_60;
    font[29].font = (uint8_t *)&font_ascii_61;
    font[30].font = (uint8_t *)&font_ascii_62;
    font[31].font = (uint8_t *)&font_ascii_63;
    font[32].font = (uint8_t *)&font_ascii_64;
    font[33].font = (uint8_t *)&font_ascii_65;
    font[34].font = (uint8_t *)&font_ascii_66;
    font[35].font = (uint8_t *)&font_ascii_67;
    font[36].font = (uint8_t *)&font_ascii_68;
    font[37].font = (uint8_t *)&font_ascii_69;
    font[38].font = (uint8_t *)&font_ascii_70;
    font[39].font = (uint8_t *)&font_ascii_71;
    font[40].font = (uint8_t *)&font_ascii_72;
    font[41].font = (uint8_t *)&font_ascii_73;
    font[42].font = (uint8_t *)&font_ascii_74;
    font[43].font = (uint8_t *)&font_ascii_75;
    font[44].font = (uint8_t *)&font_ascii_76;
    font[45].font = (uint8_t *)&font_ascii_77;
    font[46].font = (uint8_t *)&font_ascii_78;
    font[47].font = (uint8_t *)&font_ascii_79;
    font[48].font = (uint8_t *)&font_ascii_80;
    font[49].font = (uint8_t *)&font_ascii_81;
    font[50].font = (uint8_t *)&font_ascii_82;
    font[51].font = (uint8_t *)&font_ascii_83;
    font[52].font = (uint8_t *)&font_ascii_84;
    font[53].font = (uint8_t *)&font_ascii_85;
    font[54].font = (uint8_t *)&font_ascii_86;
    font[55].font = (uint8_t *)&font_ascii_87;
    font[56].font = (uint8_t *)&font_ascii_88;
    font[57].font = (uint8_t *)&font_ascii_89;
    font[58].font = (uint8_t *)&font_ascii_90;
    font[59].font = (uint8_t *)&font_ascii_91;
    font[60].font = (uint8_t *)&font_ascii_92;
    font[61].font = (uint8_t *)&font_ascii_93;
    font[62].font = (uint8_t *)&font_ascii_94;
    font[63].font = (uint8_t *)&font_ascii_95;
    font[64].font = (uint8_t *)&font_ascii_96;
    return 0;
}