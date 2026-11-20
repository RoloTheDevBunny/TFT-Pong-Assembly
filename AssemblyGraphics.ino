#include <TFT_eSPI.h>

constexpr int W = 240;
constexpr int H = 240;
constexpr unsigned PADDLE_INDEX = 0; // 0 = lower left, 9 = upper left
static_assert(PADDLE_INDEX < 10, "Choose a paddle index from 0 to 9");

TFT_eSPI tft;

uint32_t framebuffer[W * H] __attribute__((aligned(4)));
uint16_t lineBuffer[W];

uint16_t rgb888To565(uint32_t color) {
  return ((color >> 8) & 0xF800) |
         ((color >> 5) & 0x07E0) |
         ((color >> 3) & 0x001F);
}

void present() {
  tft.startWrite();
  tft.setAddrWindow(0, 0, W, H);

  for (int y = 0; y < H; y++) {
    int rowStart = y * W;

    for (int x = 0; x < W; x++) {
      lineBuffer[x] = rgb888To565(framebuffer[rowStart + x]);
    }

    tft.pushPixels(lineBuffer, W);
  }

  tft.endWrite();
}

// a0 = framebuffer address, a1 = width, a2 = height, a3 = paddle index
__attribute__((naked, noinline))
void render(uint32_t* buffer, uint32_t width, uint32_t height, uint32_t paddleIndex) {
  asm volatile(
    R"(
    mv   t0, a0
    li   t1, 0

loopY:
    bge  t1, a2, diagonal

    li   t2, 0

    slli t3, t1, 8
    ori  t3, t3, 0x80

loopX:
    bge  t2, a1, endLoopX

    sw   t3, 0(t0)

    addi t0, t0, 4
    addi t2, t2, 1
    j    loopX

endLoopX:
    addi t1, t1, 1
    j    loopY

diagonal:
    mv   t0, a0
    li   t1, 0

    addi t2, a1, 1
    slli t2, t2, 2

dloop:
    bge  t1, a2, done

    li   t3, 0x00FFFFFF
    sw   t3, 0(t0)

    add  t0, t0, t2
    addi t1, t1, 1
    j    dloop

done:
    # Each row stores x0, y0, x1, y1 as four unsigned bytes.
    la   t0, leftPositions
    slli t1, a3, 2
    add  t0, t0, t1
    lbu  t1, 0(t0)      # current x
    lbu  t2, 1(t0)      # current y
    lbu  t3, 2(t0)      # end x
    lbu  t4, 3(t0)      # end y

    # Bresenham line: a2 = abs(dx), a3 = -abs(dy).
    sub  a2, t3, t1
    li   a4, 1
    bgez a2, .LxReady
    neg  a2, a2
    li   a4, -1
.LxReady:
    sub  a3, t4, t2
    li   a5, -1
    blez a3, .LyReady
    neg  a3, a3
    li   a5, 1
.LyReady:
    add  a6, a2, a3     # error
    li   a7, 0x00FFFFFF # white paddle
.Lpixel:
    mul  t0, t2, a1
    add  t0, t0, t1
    slli t0, t0, 2
    add  t0, a0, t0
    sw   a7, 0(t0)
    bne  t1, t3, .Ladvance
    beq  t2, t4, .LpaddleDone
.Ladvance:
    slli t5, a6, 1
    blt  t5, a3, .LstepY
    add  a6, a6, a3
    add  t1, t1, a4
.LstepY:
    bgt  t5, a2, .Lpixel
    add  a6, a6, a2
    add  t2, t2, a5
    j    .Lpixel
.LpaddleDone:
    ret

    .pushsection .rodata
leftPositions:
    .byte 97,217,87,215
    .byte 70,207,62,201
    .byte 48,189,41,182
    .byte 31,166,27,157
    .byte 22,139,20,129
    .byte 20,111,22,101
    .byte 27,83,31,74
    .byte 41,58,48,51
    .byte 62,39,70,33
    .byte 87,25,97,23
    .popsection
    )"
  );
}

void setup() {
  tft.init();
  tft.setRotation(0);
  tft.setSwapBytes(true);

  render(framebuffer, W, H, PADDLE_INDEX);
  present();
}

void loop() {}
