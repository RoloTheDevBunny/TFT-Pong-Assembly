// Check User_Setup.h for pins. Ignore the touch errors, check Tools > CPU Architecture for most issues

#include <TFT_eSPI.h>

using RarsPixel = uint32_t;
using TftPixel = uint16_t;

extern "C" {
  volatile uint8_t controlInputs = 0xFF; // input positions for left, right
  volatile uint8_t ballX = 120;
  volatile uint8_t ballY = 120;
  volatile int8_t ballVelocityX = 1;
  volatile int8_t ballVelocityY = 1;
}

constexpr int screenWidth = 240;
constexpr int screenHeight = 240;
constexpr int circleRadius = 20;

constexpr RarsPixel white = 0x00FFFFFF;

constexpr int leftPot = 27;
constexpr int rightPot = 26;

TFT_eSPI display = TFT_eSPI();

RarsPixel framebuffer[screenWidth * screenHeight];
TftPixel displayLine[screenWidth];

TftPixel rarsToTFT(RarsPixel color) { // converts the byte per color channel frame buffer in rars
  uint8_t red = (color >> 16); // shift over the first two channels
  uint8_t green = (color >> 8) & 0xFF; // shift & mask
  uint8_t blue = color & 0xFF;

  return display.color565(red, green, blue);
}

void showFramebuffer() {
  display.startWrite();
  display.setAddrWindow(0, 0, screenWidth, screenHeight);

  for (int y = 0; y < screenHeight; y++) {
    RarsPixel* sourceLine = framebuffer + y * screenWidth;

    for (int x = 0; x < screenWidth; x++) {
      displayLine[x] = rarsToTFT(sourceLine[x]);
    }

    display.pushPixels(displayLine, screenWidth);
  }

  display.endWrite();
}

__attribute__((naked, noinline)) // special instruction to parse assembly
void render(
  RarsPixel* pixelMemory, // s0
  uint32_t width,     // s1
  uint32_t height,    // s2
  uint32_t radius,    // s5
  RarsPixel circleColor   // s6
) {
  asm volatile(
    R"(
CopySToStack:
  addi sp, sp, -32

  sw   ra,  0(sp)
  sw   s0,  4(sp)
  sw   s1,  8(sp)
  sw   s2, 12(sp)
  sw   s3, 16(sp)
  sw   s4, 20(sp)
  sw   s5, 24(sp)
  sw   s6, 28(sp)


  mv s0, a0     # framebuffer base
  mv s1, a1     # screen width
  mv s2, a2     # screen height
  mv s5, a3     # radius
  mv s6, a4     # color


  la a0, ballVelocityX
  lb a1, 0(a0)
  la a0, ballX
  lbu s3, 0(a0)

  add s3, s3, a1   # ball X
  sb s3, 0(a0)

  la a0, ballVelocityY
  lb a1, 0(a0)
  la a0, ballY
  lbu s4, 0(a0)

  add s4, s4, a1   # ball y
  sb s4, 0(a0)

  j RenderBackground

# x0, y0, x1, y1
leftPositions:
  .byte 126, 223,  77, 214
  .byte 107, 222,  61, 204
  .byte 89, 218,  46, 192
  .byte 71, 211,  34, 177
  .byte 55, 200,  25, 160
  .byte 42, 187,  19, 142
  .byte 31, 171,  17, 123
  .byte 23, 154,  18, 104
  .byte 18, 136,  23, 86
  .byte 17, 117,  31, 69
  .byte 19, 98,  42, 53
  .byte 25, 80,  55, 40
  .byte 34, 63,  71, 29
  .byte 46, 48,  89, 22
  .byte 61, 36,  107, 18
  .byte 77, 26,  126, 17

rightPositions:
  .byte 114, 223,  163, 214
  .byte 133, 222,  179, 204
  .byte 151, 218,  194, 192
  .byte 169, 211,  206, 177
  .byte 185, 200,  215, 160
  .byte 198, 187,  221, 142
  .byte 209, 171,  223, 123
  .byte 217, 154,  222, 104
  .byte 222, 136,  217, 86
  .byte 223, 117,  209, 69
  .byte 221, 98,  198, 53
  .byte 215, 80,  185, 40
  .byte 206, 63,  169, 29
  .byte 194, 48,  151, 22
  .byte 179, 36,  133, 18
  .byte 163, 26,  114, 17

RenderBackground:
  mv   t0, s0
  li   t1, 0
  li   t2, 0

RBLoopY:
  bge  t2, s2, RenderCircle

  li   t1, 0
  slli t3, t2, 8

RBLoopX:
  bge  t1, s1, RBEndLoopX

  sw   t3, 0(t0)

  addi t0, t0, 4
  addi t1, t1, 1
  addi t3, t3, 1
  j  RBLoopX

RBEndLoopX:
  addi t2, t2, 1
  j  RBLoopY

RenderCircle:
  mul  t4, s5, s5
  sub  t1, s4, s5

RCLoopY:
  add  t2, s4, s5
  bgt  t1, t2, RenderLeftPaddle

  sub  t5, t1, s4
  mul  t5, t5, t5

  sub  t0, s3, s5

RCLoopX:
  add  t2, s3, s5
  bgt  t0, t2, RCEndLoopX

  sub  t6, t0, s3
  mul  t6, t6, t6
  add  t6, t6, t5

  bgtu t6, t4, RCSkipPixel

  mul  t6, t1, s1
  add  t6, t6, t0
  slli t6, t6, 2
  add  t6, t6, s0

  sw   s6, 0(t6)

RCSkipPixel:
  addi t0, t0, 1
  j  RCLoopX

RCEndLoopX:
  addi t1, t1, 1
  j  RCLoopY

RenderLeftPaddle:
  la   t0, controlInputs   # address of control byte
  lbu  t1, 0(t0)      # t1 = control input bytes
  srli t1, t1, 4      # t1 = upper 4 bits

  la t0, leftPositions  # address of first line
  slli t1, t1, 2      # * 4 because there are 4 bytes per line
  add a0, t0, t1      # a0 = baseAddress + positionNum
  li a1, 0x00000000     # black color

  jal  ra, DrawLine

RenderRightPaddle:
  la   t0, controlInputs   # address of control byte
  lbu  t1, 0(t0)      # t1 = control input bytes
  andi t1, t1, 0x0F      # t1 = lower 4 bits

  la t0, rightPositions  # address of first line
  slli t1, t1, 2      # * 4 because there are 4 bytes per line
  add a0, t0, t1      # a0 = baseAddress + positionNum
  li a1, 0x00FF0000     # red color

  jal  ra, DrawLine

RenderDone:
  lw   ra,  0(sp)
  lw   s0,  4(sp)
  lw   s1,  8(sp)
  lw   s2, 12(sp)
  lw   s3, 16(sp)
  lw   s4, 20(sp)
  lw   s5, 24(sp)
  lw   s6, 28(sp)

  addi sp, sp, 32
  ret


# a0 = address of x0, y0, x1, y1
# a1 = line color

DrawLine:
  lbu  a2, 0(a0)       # x0
  lbu  a3, 1(a0)       # y0
  lbu  a4, 2(a0)       # x1
  lbu  a5, 3(a0)       # y1

  sub  t0, a4, a2      # dx = x1 - x0
  bge  t0, zero, DXPositive
  sub  t0, zero, t0    # dx = -dx

DXPositive:
  blt  a2, a4, StepXRight
  li   t1, -1        # stepX = -1
  j  FindDY

StepXRight:
  li   t1, 1         # stepX = 1

FindDY:
  sub  t2, a5, a3      # dy = y1 - y0
  bge  t2, zero, DYPositive
  sub  t2, zero, t2    # dy = -dy

DYPositive:
  blt  a3, a5, StepYDown
  li   t3, -1        # stepY = -1
  j  NegateDY

StepYDown:
  li   t3, 1         # stepY = 1

NegateDY:
  sub  t2, zero, t2    # dy = -abs(dy)
  add  t4, t0, t2      # error = dx + dy

LineLoop:
  mul  t6, a3, s1      # y0 * width
  add  t6, t6, a2      # y0 * width + x0
  slli t6, t6, 2       # pixel index * 4
  add  t6, t6, s0      # framebuffer address

  sw   a1, 0(t6)       # write passed color

  bne  a2, a4, KeepDrawing
  beq  a3, a5, LineDone

KeepDrawing:
  slli t5, t4, 1       # error * 2

  bge  t5, t2, MoveX
  j  CheckY

MoveX:
  add  t4, t4, t2
  add  a2, a2, t1      # x0 += stepX

CheckY:
  ble  t5, t0, MoveY
  j  LineLoop

MoveY:
  add  t4, t4, t0
  add  a3, a3, t3      # y0 += stepY
  j  LineLoop

LineDone:
  ret
    )"
  );
}

void initializeDisplay() {
  display.init();
  display.setRotation(0);
  display.setSwapBytes(true);
}

void runFrame() {
  int leftVal = analogRead(leftPot)*15/1000;
  int rightVal = analogRead(rightPot)*15/1000;
  Serial.print("Left: ");
  Serial.println(leftVal);
  Serial.print("Right: ");
  Serial.println(rightVal);
  controlInputs = leftVal*0x10 + rightVal;
  render(
    framebuffer,
    screenWidth,
    screenHeight,
    circleRadius,
    white
  );

  showFramebuffer();
}

void setup() {
  pinMode(leftPot, INPUT);
  pinMode(rightPot, INPUT);
  Serial.begin(9600);
  initializeDisplay();
  for (int i=0; i<75; i++) {
    runFrame();
  }
}

void loop() {
}