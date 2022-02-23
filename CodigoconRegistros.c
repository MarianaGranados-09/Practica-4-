#Include <18F4550.h>
#Include <math.h> //Usamos la libreria Math.h para usar comandos como Sqrt y Log10
#Fuses HS, NOWDT, NOPROTECT, INTRC, NOBROWNOUT
#Use delay (clock=8M, crystal=8M)

//Registros del ADC.
#BYTE ADCON0 = 0xFC2
#BYTE ADCON1 = 0xFC1
#BYTE ADCON2 = 0xFC0
#BYTE ADRESH = 0xFC4
#BYTE ADRESL = 0xFC3

//Registros del timer0.
#BYTE TMR0H=0xFD7
#BYTE TMR0L=0xFD6
#BYTE T0CON=0xFD5

//Registros básicos. 
#BYTE PORTB = 0xF81
#BYTE TRISB = 0xF93

#BYTE PORTC = 0xF82
#BYTE TRISC = 0xF94

#BYTE TRISD = 0xF95
#BYTE PORTD = 0xF83

//Variables del ADC.
Int16 Menor = 0;
Int16 Mayor = 0;
Int16 Lectura = 0;
Int16 DistanciaADC = 0;

//Variables del ultrasónico (tiempoCM, tiempoS, distancia).
int16 TiempoS=0;
//int16 TiempoCM=0;
int16 DistanciaULT=0;

//Variables de los displays.
Int8 DecenasADC, UnidadesADC, DecenasULT, UnidadesULT, TiempoADC, TiempoULT;
Int8 Numeracion []={0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

Void DisplaysADC(DecenasADC, UnidadesADC);
Void DisplaysULT(DecenasULT, UnidadesULT);
Void ADC();
Void Ultra();

#define trigger pin_c1
#define echo pin_c0

Void main ()
{
   //Configuramos el ADC.
   ADCON0 = 0B00000011; //Bit 0    Activamos el ADC (1).
                        //Bit 1    Ponemos conversión en progreso (1). 
                        //Bit 5-2  seleccionamos el Channel 0 (AN0) = (000).
                        //Bit 7-6  Se leé como "0".
                        
   ADCON1 = 0B00111110; //Bit 0-3  Configuramos el puerto AN0 (1110).
                        //Bit 4    Voltaje positivo de referencia externo (1).
                        //Bit 5    Voltaje negativo de referencia externo (1).
                        //Bit 7-6  Se leé como "0".
                        
   ADCON2 = 0B10000000; //Bit 0-2  Ciclos del reloj, 2 ciclos = (000).
                        //Bit 5-3  Tiempo de adquisición de los datos, manera instantanea = (000).
                        //Bit 6    Se leé como "0".
                        //Bit 7    Tipo de formato, justificado a la derecha = (1).
   
   //Configuración del Ultrasónico.
   T0CON = 0B01000000;  //Bit 7 Timer OFF (0) ON (1).
                        //Bit 6 timer0 se configura de 8 bits (1).
                        //Bit 5 ciclo de reloj con instruccion interna...?
                        //Bit 4 Contar cuando hay un flanco de subida
                        //Bit 3 El prescaler esta definido
                        //Bit 2-0: 000 para un valor de prescale de 1:2
   
   //Configuración de los puertos.
   TRISB = 0x00;        //Puerto D como salida para sacar la secuencia.
   TRISD = 0x00;        //Puerto B como salida para controlar el multiplexado de los displays.
   TRISC = 0x01;
   
   //Los registros donde se guarda el resultado se inicializan
   //TMR0H=0x00;
   TMR0L=0x00;
   
   //Pin C0 como entrada para recibir la señal del echo; Pin C1 como salida para enviar
   //la señal de trigger
   
    //setup_timer_0(T0_INTERNAL|T0_DIV_1);
    //T0 para configurar el reloj interno del timer0
    //T0_DIV_1 para 
    //setup_timer_0(RTCC_DIV_2) para el prescaler 1:2
   
   While (True)
   {
      ADC();
      Ultra();
   }
}

Void ADC()
{
      //0011 Realizar conversion y habilitar modulo
     ADCON0=0x03;
     Delay_us(10);
     Menor = ADRESL; //Mandamos los primero 8 bits al ADRESL;
     Mayor = ADRESH; //Mandamos los bits mas significativos al ADRESH;
     Lectura = (Mayor<<8)+Menor;
     
     ADCON0=0x00; //Deshabilita, ya se termino de realizar la conversion y entonces se apaga para retener la informacion
     
     //DistanciaADC = pow(10,(log10(Lectura/3085.6)/-0.827));   //Linea de tendencia Potencial.
     //DistanciaADC = ((Lectura-528.71)/-12.478);              //Linea de tendencia Lineal.
     //DistanciaADC = -sqrt(1.617*Lectura-314.657)+30.086;     //Linea de tendencia Polinómica.
     DistanciaADC = -0.29787*sqrt(6.1744*Lectura-2021.9)+30.094;               //Linea de tendencia Polinómica Voltajes externos.
     
      if (10<=DistanciaADC && DistanciaADC<=80 ) //Condicion para establecer un limite en la capaciadad de medicion del sensor
      {
         DecenasADC = DistanciaADC/10;    //Valor del display decena
         UnidadesADC = DistanciaADC - (DecenasADC*10);
         //Unidades = Distancia%10 + 0.5;
         DisplaysADC(DecenasADC, UnidadesADC); //Llamamos funcion para ingresar valores en el display
      }
}

Void Ultra()
{
   //Enviar un alto al pin del trigger
   PORTC=0b00000010;
   //output_high(trigger);
   //Esperar 10 microsegundos
   delay_us(10);
   //Enviar un bajo al pin del trigger
   PORTC=0b00000000;
   //output_low(trigger);
   //Mientras que no haya un flanco de subida en el pin del echo, esperar
   while(!(PORTC & 0b00000001));
   //while(!input(echo));
   //Cuando se detecte un flanco de subida en el pin del echo, inicializar timer y registro TMR0L
   TMR0L=0;
   T0CON=0b10000000;
   //set_timer0(0);
   while(PORTC & 0b00000001);
   //while(input(echo));
   //Cuando se deje de detectar el flanco de subida en el pin del echo, se rompe el ciclo while y
   //Apaga el timer0 y recolecta el valor del registro en TMR0L 
   T0CON=0x00; 
   TiempoS=TMR0L; //Instruccion en C: get_timer0();
   //TiempoS=get_timer0();
   
    
   //d=((t/2)/60)+1.5;            //Formula para la simulación
   //d=t/58;
   //d=(t*0.034)/2;
   DistanciaULT=((TiempoS)/29.155)*4;
   //DistanciaULT=TiempoS*0.01715;

   DecenasULT = DistanciaULT/10;    //Valor del display decena
   UnidadesULT = DistanciaULT - (DecenasULT*10);
   //Unidades = Distancia%10 + 0.5;
   DisplaysULT(DecenasULT, UnidadesULT); //Llamamos funcion para ingresar valores en el display
}

Void DisplaysADC(DecenasADC, UnidadesADC)
{
   //Intercalamos los encedidos de los displays.
   For (TiempoADC=0; TiempoADC<2; TiempoADC++)
   {
      
      PORTB = 0b00001101;
      PORTD = Numeracion[DecenasADC];
      delay_us(500);
      PORTB = 0b00001110;
      PORTD = Numeracion[UnidadesADC];
      delay_us(400);
   }
   TiempoADC=0;
}

Void DisplaysULT(DecenasULT, UnidadesULT)
{
   //Intercalamos los encedidos de los displays.
   For (TiempoULT=0; TiempoULT<2; TiempoULT++)
   {
      PORTB = 0b00000111;
      PORTD = Numeracion[DecenasULT];
      delay_us(500);
      PORTB = 0b00001011;
      PORTD = Numeracion[UnidadesULT];
      delay_us(400);
   }
   TiempoULT=0;
}
