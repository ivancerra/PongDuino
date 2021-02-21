/**************************************************************************
 *                                   ·
 *                                   ·
 *                                   ·                      |
 *                                   · 
 *             o                     ·
 *                                   · 
 *  |                                ·
 *                                   · 
 *                                   · 
 *                                   · 
 *                      <<<<<<<< P  O  N  G >>>>>>>
 *    <<<<<<<< Copyright Ivan Cerra De Castro - Febrero 2021 >>>>>>>>>>>
 *           <<<<<<<< https://www.ivancerra.com >>>>>>>>>>>
 * 
 *                 Video: https://youtu.be/ATRtT_YCd1o
 * ************************************************************************
 * Emulacion de Atari Pong 1972 para Arduino - Allan Alcorn  & Nolan Bushnell
 * 
 * Componentes usados y conexiones:
 * 1. Arduino UNO.
 * 2. TFT AZDelivery SPI ST7735 128x160.
 *    Conexiones: 
 *        · LED -> 3.3V
 *        · SCK -> 13
 *        · SDA -> 11
 *        · A0 -> 9
 *        · RESET -> 8
 *        · CS -> 10
 *        · GND -> GND
 *        · VCC -> 5V
 * 3. Buzzer Pasivo. 
 *    Conexiones:
 *        · Puerto -> 5
 * 4. Boton Player 1 con Resistencia 1K en PullDown.
 *    Conexiones:
 *        · Puerto -> 2
 * 5. Boton Player 2 con Resistencia 1K en PullDown.
 *    Conexiones:
 *        · Puerto -> 4
 * 6. Potenciometro Player 1.
 *    Conexiones:
 *        · Puerto -> A0
 * 7. Potenciometro Player 2.
 *    Conexiones:
 *        · Puerto -> A1
 **************************************************************************/

// Libreria Grafica
#include <Adafruit_GFX.h>
// HAL de Chip ST7735 TFT Controller    
#include <Adafruit_ST7735.h> 
// Libreria del Puerto Serie de Arduino
#include <SPI.h>

// Puertos para TFT
#define TFT_CS        10
#define TFT_RST        8 
#define TFT_DC         9

// Puerto de Potenciometro
#define POTENTIOMETER1 A0
#define POTENTIOMETER2 A1

// Puerto para Buzzer
#define BUZZER 5

// Puerto para Player 1 Coin
#define PLAYER1_COIN 2
// Puerto para Player 2 Coin
#define PLAYER2_COIN 4

// Sonidos
#define SOUND_BALL_WALL 500
#define SOUND_BALL_RACKET 1000
#define SOUND_BALL_POINT_HI 150
#define SOUND_BALL_POINT_LO 90

// Colores
#define WHITE ST7735_WHITE
#define BLACK ST7735_BLACK

// Dip Switches de Juego ;)
#define IA_INI 0
#define SPEED_INI 25
#define POINTS_MATCH 11

// Estructura para tamaño
typedef struct tagSize
{
  int width;
  int height;
}Size;

// Estructura para coordenadas
typedef struct tagPoint
{
  int x;
  int y;
}Point;

// Estructura para velocidad
typedef struct tagSpeed
{
  int dx;
  int dy;
}Speed;

// Estructura del juego
typedef struct tagGame
{
  // Pantalla
  Size screen;
  Size screenHalf;

  // Pala
  Size paddle;
  Size paddleHalf;
  int paddle8Seg;
  
  // Punto para numeros
  int dotSize;

  // Margen para posicionar raquetas
  int margin;

  // Pelota
  int ballSize;
  Point ball;
  Point ballOld;
  Speed ballSpeed;

  // Players
  Point player[2];
  Point playerOld[2];

  // Scores
  Point lblPlayerScore[2];
  int playerScore[2];

  // Estado del juego
  bool playing;
  bool twoPlayers;

  // Toques recibidos en un punto
  int hitsBall;
  // Tiempo para que aparezca la pelota
  int timeBall;
  // Jugador hacia el que va la pelota
  int ballToPlayer;

}Game;

// Inicializacion de TFT 1.8 128 x 160 
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Variables de Juego
Game _game;

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  I N I C I A L I Z A C I O N
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Rutinas de inicializacion
 **************************************************************************/
void setup(void) 
{
  // Inicializamos sensores
  setupSensors();

  // Inicializamos juego
  setupGame();

  // Borramos la pantalla
  tft.fillScreen(BLACK);

  // Empieza en Demo
  initDemo();
}

/**************************************************************************
  Inicializacion de sensores
 **************************************************************************/
void setupSensors()
{
  // Inicializamos ST7735S chip, black tab
  tft.initR(INITR_BLACKTAB); 

  // Entrada para Player 1 Coin
  pinMode(PLAYER1_COIN, INPUT);

  // Salida para Buzzer
  pinMode(BUZZER, OUTPUT);     
}

/**************************************************************************
  Inicializacion de variables de juego
 **************************************************************************/
void setupGame()
{
  // Dimension de Pantalla
  _game.screen.width = 128;
  _game.screen.height = 160;
  _game.screenHalf.width = _game.screen.width / 2;
  _game.screenHalf.height = _game.screen.height / 2;

  // Datos de las palas
  _game.paddle.width = max(((_game.screen.width / 16) / 8) * 8, 1);
  _game.paddle.height = _game.paddle.width / 4;
  _game.paddleHalf.width = _game.paddle.width / 2;
  _game.paddleHalf.height = _game.paddle.height / 2;
  _game.paddle8Seg = _game.paddle.width / 8;

  // Margen para las palas con respecto al borde
  _game.margin = 15;

  // Tamaño de la pelota, cogemos el grosor de la pala
  _game.ballSize = _game.paddle.height;

  // Tamaño de punto de fuente, cogemos el grosor de la pala
  _game.dotSize = _game.paddle.height;

  // Posicion de los marcadores
  _game.lblPlayerScore[0].x = _game.dotSize * 10;
  _game.lblPlayerScore[0].y = (_game.screen.height * 3) / 4 - _game.dotSize * 6;
  _game.lblPlayerScore[1].x = _game.dotSize * 10;
  _game.lblPlayerScore[1].y = _game.screen.height / 4;

  // Ponemos la posicion de los jugadores
  readPlayerPotentiometer(POTENTIOMETER1, &(_game.player[0]));
  _game.player[0].y = _game.screen.height - _game.margin;
  readPlayerAutomatic(&(_game.player[1]));
  _game.player[1].y = _game.margin;

  // Inicializamos Score
  _game.playerScore[0] = 0;
  _game.playerScore[1] = 0;

  // Iniciamos la generacion de numeros aleatorios
  randomSeed(analogRead(POTENTIOMETER1));

  // Colocamos la pelota
  initBall(0);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  P R O G R A M A   P R I N C I P A L
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Bucle principal
 **************************************************************************/
void loop() 
{
  // Guardamos las posiciones de los actores, no hay memoria para pagina oculta ;)
  oldPositions();

  // Comprobamos si han echado moneda
  checkCoin();

  // Leemos el player 1
  readPlayerPotentiometer(POTENTIOMETER1, &(_game.player[0]));
  
  // Leemos el player 2
  // Comprobamos si para un player o dos
  if(_game.twoPlayers)
  {
    readPlayerPotentiometer(POTENTIOMETER2, &(_game.player[1]));
  }
  // Para 1 Player
  else
  {
    readPlayerAutomatic(&(_game.player[1]));
  }

  // Leemos la pelota
  readBall();

  // Pintamos la pelota
  drawBall(_game.ballOld.x, _game.ballOld.y, _game.ball.x, _game.ball.y, WHITE);
    
  // Pintamos las palas
  drawPaddle(_game.playerOld[0].x, _game.player[0].x, _game.player[0].y, WHITE);
  drawPaddle(_game.playerOld[1].x, _game.player[1].x, _game.player[1].y, WHITE);
    
  // Pintamos la red
  drawNet();

  // Pintamos Score
  drawScores(WHITE);
  
  // Velocidad de juego proporcional a toques de jugador
  delay(max(SPEED_INI - _game.hitsBall, 5));
}

/**************************************************************************
  Guardamos las posiciones antiguas
 **************************************************************************/
void oldPositions()
{
  _game.playerOld[0].x = _game.player[0].x;
  _game.playerOld[0].y = _game.player[0].y;
  _game.playerOld[1].x = _game.player[1].x;
  _game.playerOld[1].y = _game.player[1].y;

  _game.ballOld.x = _game.ball.x;
  _game.ballOld.y = _game.ball.y;
}

/**************************************************************************
  Add Point
 **************************************************************************/
void addPoint(int player)
{
  // Borramos Score
  drawScores(BLACK);

  _game.playerScore[player]++;

  // Pintamos Score 
  drawScores(WHITE);

  // Sonido de punto!!!
  soundPoint();

  // Esperamos un tiempo para prepararse
  delay(100);

  // Borramos la pelota
  drawBall(_game.ballOld.x, _game.ballOld.y, _game.ball.x, _game.ball.y, BLACK);

  // Comprobamos si ha terminado el juego
  if(_game.playerScore[0] == POINTS_MATCH || _game.playerScore[1] == POINTS_MATCH)
  {
    endGame();

    return;
  }

  // Iniciamos la pelota hacia el player perdedor
  initBall(1 - player);
}

/**************************************************************************
  Comprobacion de monedas
 **************************************************************************/
void checkCoin()
{
  // Si estamos jugando no comenzamos nuevas partidas
  if(_game.playing)
  {
    return;
  }

  // Leemos Boton de Player 1
  int valuePlayer1Coin = digitalRead(PLAYER1_COIN); 
  // Leemos Boton de Player 2
  int valuePlayer2Coin = digitalRead(PLAYER2_COIN); 
 
  // Si hemos pulsado boton de Player 1, comenzamos partida
  if (valuePlayer1Coin == HIGH) 
  {
    _game.twoPlayers = false;
    initPlay();
    
    return;
  }

  // Si hemos pulsado boton de Player 2, comenzamos partida
  if (valuePlayer2Coin == HIGH) 
  {
    _game.twoPlayers = true;
    initPlay();
  }
}

/**************************************************************************
  Inicio de Demo
 **************************************************************************/
void initDemo()
{
  // Quitamos las palas
  drawPaddle(_game.playerOld[0].x, _game.player[0].x, _game.player[0].y, BLACK);
  drawPaddle(_game.playerOld[1].x, _game.player[1].x, _game.player[1].y, BLACK);
    
  // Indicamos que no estamos jugando
  _game.playing = false;
}

/**************************************************************************
  Inicio de Partida
 **************************************************************************/
void initPlay()
{
  // Borramos Score
  drawScores(BLACK);

  // Reseteamos marcadores
  _game.playerScore[0] = 0;
  _game.playerScore[1] = 0;
 
  // Sonido de partida
  soundPlay();

  // Comienza el juego  
  _game.playing = true;

  // Borramos la pelota donde estuviera
  drawBall(_game.ballOld.x, _game.ballOld.y, _game.ball.x, _game.ball.y, BLACK);

  // Colocamos Pelota
  initBall(0);
}

/**************************************************************************
  Fin de Juego
 **************************************************************************/
void endGame()
{
  // Fin del partido, dejamos en demo
  initDemo();
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   L E C T U R A   D A T O S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Leemos la Pelota
 **************************************************************************/
void readBall()
{
  // Si no debe correr la pelota
  if(_game.timeBall > 0)
  {
    _game.timeBall--;

    return;
  }

  _game.ball.x = _game.ball.x + _game.ballSpeed.dx;
  _game.ball.y = _game.ball.y + _game.ballSpeed.dy;

  // Colision con Pared
  if((_game.ball.x <= _game.ballSize && _game.ballSpeed.dx < 0) || (_game.ball.x >= _game.screen.width - _game.ballSize && _game.ballSpeed.dx > 0))
  {
    _game.ballSpeed.dx = - _game.ballSpeed.dx;

    soundBall(SOUND_BALL_WALL);
  }

  // Punto!!!!!
  if((_game.ball.y <= 0 && _game.ballSpeed.dy < 0) || (_game.ball.y >= _game.screen.height && _game.ballSpeed.dy > 0))
  {
    // Si no estamos jugando rebotamos y nos vamos
    if(!_game.playing)
    {
      _game.ballSpeed.dy = - _game.ballSpeed.dy;

      return;
    }

    int player = (_game.ball.y < _game.screenHalf.height)?0:1;

    addPoint(player);
  }
  
  // Colision con Player 1
  checkBallPaddle(1, &(_game.player[0]));

  // Colision con Player 2
  checkBallPaddle(2, &(_game.player[1]));
  
}

/**************************************************************************
  Comprobamos colision Bola - Pala
 **************************************************************************/
void checkBallPaddle(int playerNum, Point *playerPos)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  bool collissionY = abs(playerPos->y - _game.ball.y) < _game.paddle.height;
  
  // Colision con Player 
  if((collissionY && _game.ballSpeed.dy > 0 && playerNum == 1) || 
     (collissionY && _game.ballSpeed.dy < 0 && playerNum == 2))
  {
    int difX = _game.ball.x - playerPos->x;
      
    // Si la diferencia en valor absoluto es menor hay colision
    if(abs(difX) < _game.paddleHalf.width + _game.ballSize)
    {
      // Rebota de sentido
      _game.ballSpeed.dy = - _game.ballSpeed.dy;
      
      // miramos el segmento para calcular como debetener el angulo de salida
      difX = difX / _game.paddle8Seg;

      // 8 Segmentos de Pala
      // 2 1 1 0 0 1 1 2 
      // Para que haya 0 0
      if(difX > 0)
      {
        difX--;
      }

      // Para que haya 1 1   1 1
      if(difX > 0)
      {
        difX++;  
      }
      if(difX < 0)
      {
        difX--;
      }

      // Segmento golpeado
      int segment = difX / 2;

      // No permitimos angulos mayores de 45º
      _game.ballSpeed.dx = max(min(segment ,2), -2);

      // Aumentamos toques de bola por parte del jugador 
      _game.hitsBall++;

      soundBall(SOUND_BALL_RACKET);
    }
  }
}

/**************************************************************************
  Inicio de la Pelota
 **************************************************************************/
void initBall(int dirPlayer)
{
  // Inicializamos valores de la pelota
  _game.timeBall = 20;
  _game.hitsBall = 0;

  // Iniciamos coordenadas
  _game.ball.y = _game.screenHalf.height;
  _game.ball.x = _game.screenHalf.width;

  // Sentido de la y
  _game.ballSpeed.dy = (dirPlayer == 0)?2:-2;

  // Direccion de la x, aleatoria
  _game.ballSpeed.dx = 2 - random(5); 
}

/**************************************************************************
  Leemos la X del Player 1
 **************************************************************************/
void readPlayerPotentiometer(uint8_t port, Point *pos)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  int value = analogRead(port);
  
  pos->x = map(value, 0, 1023, 1, _game.screen.width - 2);
}

/**************************************************************************
  Leemos la X del Player 2 si lo gestiona la maquina
 **************************************************************************/
void readPlayerAutomatic(Point *pos)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  int difX = abs(_game.ball.x - pos->x);
  int maxDif = 2;

  // Nivel de inteligencia cuanto menor mas inteligente y rapido
  if(difX < IA_INI + _game.hitsBall)
  {
    maxDif = 1;
  }

  int offset = min(difX, maxDif);

  // Vamos hacia arriba
  if(_game.ball.x < pos->x && pos->x > _game.paddle.width)
  {
    pos->x = pos->x - offset;

    return;
  }

  // Vamos hacia abajo
  if(_game.ball.x > pos->x && pos->x < _game.screen.width - _game.paddle.width)
  {
    pos->x = pos->x + offset;

    return;
  }
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   D I B U J O   A C T O R E S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos la red
 **************************************************************************/
void drawNet()
{
  for(int i = 5; i <= _game.screen.width - 5; i += 5)
  {
    drawLineH(i, _game.screenHalf.height, 3, WHITE);
  }
}

/**************************************************************************
  Pintamos raqueta
 **************************************************************************/
void drawPaddle(int oldX, int x, int y, uint16_t color)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  // Borramos la parte de pala que no se va a pintar, posicion antigua
  if(oldX <= x)
  {
    fillRect(oldX - _game.paddleHalf.width, y - _game.paddleHalf.height, x - oldX, _game.paddle.height, BLACK);
  }
  else
  {
    fillRect(x + _game.paddleHalf.width, y - _game.paddleHalf.height, oldX - x, _game.paddle.height, BLACK);
  }

  // Pintamos la pala en la posicion nueva
  fillRect(x - _game.paddleHalf.width, y - _game.paddleHalf.height, _game.paddle.width, _game.paddle.height, color);
}

/**************************************************************************
  Pintamos Pelota
 **************************************************************************/
void drawBall(int oldX, int oldY, int x, int y, uint16_t color)
{
  // Si estamos en espera de pelota, nos vamos
  if(_game.timeBall > 0)
  {
    return;
  }

  // Borramos la posicion de la pelota antes de moverse
  fillRect(oldX - _game.ballSize / 2, oldY - _game.ballSize / 2, _game.ballSize, _game.ballSize, BLACK);

  // Pintamos la pelota en la nueva posicion
  fillRect(x - _game.ballSize / 2, y - _game.ballSize / 2, _game.ballSize, _game.ballSize, color);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   D I B U J O   M A R C A D O R E S
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos Cero
 **************************************************************************/
void drawZero(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;

  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  // --------
  fillRect(x - dot * 4, y + dot * 2, dot * 9, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  //        |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
}

/**************************************************************************
  Pintamos Uno
 **************************************************************************/
void drawOne(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;

  // --------
  fillRect(x - dot * 4, y - dot, dot * 9, dot, color);
}

/**************************************************************************
  Pintamos Dos
 **************************************************************************/
void drawTwo(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // ----
  fillRect(x - dot * 4, y - dot * 2, dot * 4, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  //          |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
  //      -----
  fillRect(x - dot, y + dot, dot * 5, dot, color);
}

/**************************************************************************
  Pintamos Tres
 **************************************************************************/
void drawThree(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  //          |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
}

/**************************************************************************
  Pintamos Cuatro
 **************************************************************************/
void drawFour(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  // ----
  fillRect(x - dot * 4, y + dot, dot * 4, dot, color);
}

/**************************************************************************
  Pintamos Cinco
 **************************************************************************/
void drawFive(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  // ----
  fillRect(x - dot * 4, y + dot, dot * 4, dot, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  //      -----
  fillRect(x - dot, y - dot * 2, dot * 5, dot, color);
  //          |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
}

/**************************************************************************
  Pintamos Seis
 **************************************************************************/
void drawSix(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  //      -----
  fillRect(x - dot, y - dot * 2, dot * 5, dot, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  //          |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
  // --------
  fillRect(x - dot * 4, y + dot, dot * 9, dot, color);
}

/**************************************************************************
  Pintamos Siete
 **************************************************************************/
void drawSeven(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
}

/**************************************************************************
  Pintamos Ocho
 **************************************************************************/
void drawEight(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  //          |
  fillRect(x + dot * 4, y - dot * 2, dot, dot * 4, color);
  // --------
  fillRect(x - dot * 4, y + dot, dot * 9, dot, color);
}

/**************************************************************************
  Pintamos Nueve
 **************************************************************************/
void drawNine(int x, int y, uint16_t color)
{
  int dot = _game.dotSize;
  
  // --------
  fillRect(x - dot * 4, y - dot * 2, dot * 9, dot, color);
  // |
  fillRect(x - dot * 4, y - dot * 2, dot, dot * 4, color);
  //     |
  fillRect(x - dot, y - dot * 2, dot, dot * 4, color);
  // ----
  fillRect(x - dot * 4, y + dot, dot * 4, dot, color);
}

// Definimos vector de funciones de pintado
void (*pDrawFunction[10]) (int x, int y, uint16_t color) = {drawZero, drawOne, drawTwo, drawThree, drawFour, drawFive, drawSix, drawSeven, drawEight, drawNine};
 
/**************************************************************************
  Pintamos Digito Score
 **************************************************************************/
void drawDigitScore(int x, int y, int digit, uint16_t color)
{
  // Validamos limites
  if(digit < 0 || digit > 9)
  {
    return;
  }
  
  // Pintamos el digito
  (*pDrawFunction[digit])(x, y, color);
}

/**************************************************************************
  Pintamos Score
 **************************************************************************/
void drawScore(int x, int y, int score, uint16_t color)
{
  // Marcadores solo se permite entre 0 y 99
  if(score < 0 || score > 99)
  {
    return;
  }

  // Primer digito de derecha a Izquierda 9_
  int digit = score % 10;

  for(int i = 0; i <= score / 10; i++)
  {
    drawDigitScore(x, y + _game.dotSize * 8 * i, digit, color);

    // Segundo Digito de derecha a izquierda _9
    digit = (score / 10) % 10;
  }
}

/**************************************************************************
  Pintamos Los dos Scores
 **************************************************************************/
void drawScores(uint16_t color)
{
  drawScore(_game.lblPlayerScore[0].x, _game.lblPlayerScore[0].y, _game.playerScore[0], color);
  drawScore(_game.lblPlayerScore[1].x, _game.lblPlayerScore[1].y, _game.playerScore[1], color);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E  A B S T R A C C I O N   G R A F I C A
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Pintamos Rectangulo relleno
 **************************************************************************/
void fillRect(int16_t x, int16_t y, int16_t width, int16_t height, uint16_t color)
{
  tft.fillRect(x, y, width, height, color);
}

/**************************************************************************
  Pintamos linea
 **************************************************************************/
void drawLineH(int16_t x, int16_t y, int16_t width, uint16_t color)
{
  tft.drawFastHLine(x, y, width, color);
}

/**************************************************************************
/**************************************************************************
//  R U T I N A S   D E   A U D I O
/**************************************************************************
/**************************************************************************
/**************************************************************************
  Sonido de pelota
 **************************************************************************/
void soundBall(unsigned int tono)
{
  // Si no estamos jugando nos vamos
  if(!_game.playing)
  {
    return;
  }

  tone(BUZZER, tono, 20);
}

/**************************************************************************
  Sonido de punto
 **************************************************************************/
void soundPoint()
{
  for(int i = 0; i < 10; i++)
  {
    if(i % 2 == 0)
    {
      tone(BUZZER, SOUND_BALL_POINT_HI, 50);
    }
    else
    {
      tone(BUZZER, SOUND_BALL_POINT_LO, 75);
    }
  }
}

/**************************************************************************
  Sonido de Empezar Partida
 **************************************************************************/
void soundPlay()
{
  for(int i = 0; i < 50; i++)
  {
    tone(BUZZER, 2000 + i * 10 , 30);
  }
}