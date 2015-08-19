#if defined(ARDUINO) && ARDUINO >= 100
  #include "Arduino.h"
#else
  #include "WProgram.h"
#endif

#include "DRV8711.h"


DRV8711::DRV8711 (byte CSpin)
{
  _CSpin = CSpin ;
  control_reg = 0x000 ;
}

char DRV8711::transfer (char data)
{
  SPDR = data ;                    // Start the transfer
  int timeout = 0 ;
  while (!(SPSR & (1<<SPIF)))     // Wait for the end of the transfer
  {
    timeout ++ ; 
    if (timeout > 100)
      break ;
  }
  return SPDR;                    // return the received byte
}

void DRV8711::set_reg (byte reg, int val)
{
  val &= 0xFFF ;
  digitalWrite (_CSpin, HIGH) ;
  delayMicroseconds (1) ;
  transfer ((val >> 8) | ((reg & 0x07) << 4)) ;
  transfer (val & 0xFF) ;
  digitalWrite (_CSpin, LOW) ;
  delayMicroseconds (1) ;
}

int DRV8711::get_reg (byte reg)
{
  digitalWrite (_CSpin, HIGH) ;
  delayMicroseconds (1) ;
  int res = transfer (((reg & 0x7) << 4) | 0x80) ;
  res = (res << 8) | (0xFF & transfer (0)) ;
  digitalWrite (_CSpin, LOW) ;
  delayMicroseconds (1) ;
  return res & 0xFFF ;
}

byte DRV8711::get_status ()
{
  return get_reg (STATUS_REG) ;
}

void DRV8711::clear_status ()
{
  set_reg (STATUS_REG, 0x00) ;
}

static void EEPROMwrite (unsigned int addr, byte value)
{
  byte old = EEPROM.read (addr) ;
  if (old != value)
    EEPROM.write (addr, value) ;
}

void DRV8711::setup_spi ()
{
  digitalWrite (pinSS, HIGH) ;  pinMode (pinSS, OUTPUT) ;
  byte speedcode = 1 ; // 1MHz ? 4MHz
  SPSR = (speedcode >> 2) & 1 ;
  SPCR = (1<<MSTR) | (speedcode & 3) ;
  SPCR |= 1<<SPE ;
  pinMode (pinSCLK, OUTPUT) ; digitalWrite (pinSCLK, LOW) ;
  pinMode (pinMOSI, OUTPUT) ; digitalWrite (pinMOSI, LOW) ;
  pinMode (_CSpin, OUTPUT) ;  digitalWrite (_CSpin, LOW) ;

  byte junk = SPSR ;
  junk |= SPDR ;
}

void DRV8711::save_to_eeprom (int eeprom_addr)
{
  for (byte reg = 0 ; reg < 7 ; reg++)
  {
    int val = get_reg (reg) ;
    EEPROMwrite (eeprom_addr + 2*reg, val >> 8) ;
    EEPROMwrite (eeprom_addr + 2*reg + 1, val & 0xFF) ;
  }
}

void DRV8711::load_from_eeprom (int eeprom_addr)
{
  set_enable (false) ;
  for (char reg = 6 ; reg >= 0 ; reg--)  // write CTRL_REG last (enable bit)
  {
    byte high = EEPROM.read (eeprom_addr + 2*reg) ;
    byte low  = EEPROM.read (eeprom_addr + 2*reg + 1) ;
    int val = high << 8 ;
    val |= low ;
    set_reg (reg, val) ;
    if (reg == CTRL_REG)
      control_reg = val ;
  }
}

void DRV8711::begin (byte drive, 
                     unsigned int microsteps,
                     int eeprom_addr)
{
  setup_spi () ;
  // restore settings from eeprom  (should test for unprogrammed eeprom)
  load_from_eeprom (eeprom_addr) ;
  // handle drive and microsteps
  drive_level = drive ;
  int microstep_code = 0 ;
  while (microsteps > 1)
  {
    microsteps >>= 1 ;
    microstep_code ++ ;
  }

  // set the microstepping mode field
  control_reg = (control_reg & 0xFC3) | ((microstep_code & 0xF) << 3) ;
  set_reg (CTRL_REG, control_reg) ;

  // TORQUE REG  -ssstttttttt
  // sss = (50,100,200,300,400,600,800,1000)us, backEMF sample threshhold
  // tttttttt = torque level
  set_reg (TORQUE_REG, (get_reg (TORQUE_REG) & 0xF00) | (drive & DRV8711TRQ_TORQUE_MASK)) ;

  clear_status () ;
  set_enable (true) ;
}

void DRV8711::begin (byte drive, 
                     unsigned int microsteps, 
                     byte decay_mode,
                     byte gate_speed, 
                     byte gate_drive,
                     byte deadtime)
{
  setup_spi () ;
  drive_level = drive ;
  int microstep_code = 0 ;
  while (microsteps > 1)
  {
    microsteps >>= 1 ;
    microstep_code ++ ;
  }

  int decay_time = 0x20 ;


  // CONTROL REG    ddggsmmmmSDE
  // dd = deadtime (400, 450, 650, 850)ns
  // gg = Igain (5, 10, 20, 40)   note threshold after gain = 2.75V, so (550mV, 275mV, 138mV, 69mV)
  // s = (internal, external) stall detect
  // mmmm = (full, half, 1/4, 1/8, 1/16, 1/32, 1/64, 1/128, 1/256,...) microstepping mode
  // S = (nop, forcestep)  - self-clears
  // D = (normal, reverse) XORed with direction pin
  // E = (off, on)  enable motor(s)
  control_reg = ((deadtime & 3) << 10) | 
                ((microstep_code & 0xF) << 3) |
                DRV8711CTL_IGAIN_10 | DRV8711CTL_STALL_INTERNAL ;
  set_reg (CTRL_REG,   control_reg) ;

  // TORQUE REG  -ssstttttttt
  // sss = (50,100,200,300,400,600,800,1000)us, backEMF sample threshhold
  // tttttttt = torque level
  set_reg (TORQUE_REG, DRV8711TRQ_BEMF_50us | (drive & DRV8711TRQ_TORQUE_MASK)) ;

  // OFF REG  ---moooooooo
  // m = (stepper, DCmotors)
  // oooooooo = off time in 500ns units (500ns..128us)
  set_reg (OFF_REG,   DRV8711OFF_STEPMOTOR | (0x030 & DRV8711OFF_OFFTIME_MASK)) ;

  // BLANK REG  ---abbbbbbbb
  // a = (off,on)  adaptive blanking time
  // bbbbbbbb = blank time in units 20ns, but 1us minimum, so 1us--5.12us
  set_reg (BLANK_REG,  DRV8711BLNK_ADAPTIVE_BLANK | (0x80 & DRV8711BLNK_BLANKTIME_MASK)) ;

  // DECAY REG  -mmmdddddddd
  // mmm = (slow, slow/mix, fast, mixed, slow/auto, auto, ...)
  // dddddddd = mixed decay time in units 0.5us--128us
  set_reg (DECAY_REG,  (decay_mode & 7) << 8 | (decay_time & DRV8711DEC_DECAYTIME_MASK)) ;

  // STALL REG  ddsstttttttt
  // dd = (1/32, 1/16, 1/8, 1/4)  divide backEMF
  // ss = (1, 2, 4, 8) steps before stall asserted
  // tttttttt = stall threshold
  set_reg (STALL_REG,  DRV8711STL_DIVIDE_8 | DRV8711STL_STEPS_1 | (0x20 & DRV8711STL_THRES_MASK)) ; 

  // DRIVE REG  HHLLhhllddoo
  // HH = (50, 100, 150, 200)mA  high gate drive level
  // LL = (100, 200, 300, 400)mA low gate drive level
  // hh = (250, 500, 1000, 2000)ns high gate active drive time
  // ll = (250, 500, 1000, 2000)ns low gate active drive time
  // dd = (1, 2, 4, 8)us  OCP deglitch time
  // oo = (250, 500, 750, 1000)mV  OCP threshold (across FET)
  set_reg (DRIVE_REG,  ((gate_drive & 3) * 0x500) |
                       ((gate_speed & 3) * 0x050) |
                       DRV8711DRV_OCP_1us | 
                       DRV8711DRV_OCP_250mV) ;

  // STATUS REG  ----LSBAUbaT
  // L = latched stall fault (write 0 to clear)
  // S = transient stall fault (self clearing)
  // B = predriver B fault (write 0 to clear)
  // A = predriver A fault (write 0 to clear)
  // U = undervoltage fault (self clearing)
  // b = B overcurrent fault (write 0 to clear)
  // a = A overcurrent fault (write 0 to clear)
  // T = overtemperature fault (self clearing)
  set_reg (STATUS_REG, 0) ;  // clear all status bits

  clear_status () ;
  set_enable (true) ;
}

void DRV8711::power (byte code)
{
  byte drive = 
    code == DRV8711_HALF ? drive_level >> 1 :
    code == DRV8711_FULL ? drive_level : 0 ;
  set_reg (TORQUE_REG, DRV8711TRQ_BEMF_50us | (drive & DRV8711TRQ_TORQUE_MASK)) ;
}

void DRV8711::end ()
{
  set_enable (false) ;
}

void DRV8711::set_enable (bool enable)
{
  set_reg (CTRL_REG, control_reg | (enable ? DRV8711CTL_ENABLE : 0)) ;
}


byte DRV8711::reg_field (int field)
{
  int msk = (1 << BITS (field)) - 1 ;
  unsigned int regval = get_reg (REG (field)) ;
  regval >>= SHF (field) ;
  return (byte) (regval & msk) ;
}

void DRV8711::write_reg_field (int field, byte val)
{
  int msk = (1 << BITS (field)) - 1 ;
  val &= msk ;
  byte reg = REG (field) ;
  byte shf = SHF (field) ;
  int regval = get_reg (reg) ;
  regval &= ~(msk << shf) ;
  regval |= val << shf ;
  set_reg (reg, regval) ;
}
