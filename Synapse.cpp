/*
        ##########    Copyright (C) 2016 Vincenzo Pacella
        ##      ##    Distributed under MIT license, see file LICENSE-SW
        ##      ##    or <http://opensource.org/licenses/MIT>
        ##      ##
##########      ############################################################# shaduzlabs.com #####*/

#include "Synapse.h"
#include "digitalWriteFast.h" // --> https://github.com/watterott/Arduino-Libs/

//--------------------------------------------------------------------------------------------------

namespace
{
  static constexpr uint8_t  k_writeChannelA = 0b01010000;
  static constexpr uint8_t  k_writeChannelB = 0b11010000;

  static constexpr unsigned k_pinChipSelectDAC = 10;

  static constexpr uint8_t  k_pinGateInA = 3;
  static constexpr uint8_t  k_pinGateInB = 2;
  static constexpr uint8_t  k_pinGateOutA = 5;
  static constexpr uint8_t  k_pinGateOutB = 4;

  static constexpr uint8_t  k_pinCVOutConfA = 6;
  static constexpr uint8_t  k_pinCVOutConfB = 7;

  static constexpr uint8_t  k_pinCVInA = A0;
  static constexpr uint8_t  k_pinCVInB = A1;
}

//--------------------------------------------------------------------------------------------------

namespace sl
{

//--------------------------------------------------------------------------------------------------

Synapse::Synapse(unsigned spiDivider_)
{
  pinModeFast(k_pinGateInA, INPUT);
  pinModeFast(k_pinGateInB, INPUT);
  pinModeFast(k_pinGateOutA, OUTPUT);
  pinModeFast(k_pinGateOutB, OUTPUT);

  pinModeFast(k_pinCVInA, INPUT);
  pinModeFast(k_pinCVInB, INPUT);
  pinModeFast(k_pinCVOutConfA, OUTPUT);
  pinModeFast(k_pinCVOutConfB, OUTPUT);

  digitalWriteFast(k_pinCVOutConfA, LOW);
  digitalWriteFast(k_pinCVOutConfB, LOW);

  pinModeFast(k_pinChipSelectDAC, OUTPUT);
  digitalWriteFast(k_pinChipSelectDAC, HIGH);

  SPI.begin();
  SPI.setBitOrder(MSBFIRST);
  SPI.setDataMode(SPI_MODE0);

  setSPIDivider(spiDivider_);
  updateCVRanges();
}

//--------------------------------------------------------------------------------------------------

unsigned Synapse::readCV(CVChannel channel_)
{
  switch (channel_)
  {
    case CVChannel::A:
    {
      return analogRead(k_pinCVInA);
    }
    case CVChannel::B:
    {
      return analogRead(k_pinCVInB);
    }
    default:
    {
      return 0U;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Synapse::writeCV(CVChannel channel_, unsigned value_)
{
  uint8_t msg1 = (uint8_t)((value_ >> 8) & 0x0F);
  uint8_t msg2 = (uint8_t)(value_ & 0xFF);

  switch (channel_)
  {
    case CVChannel::A:
    {
      msg1 |= k_writeChannelA;
    }
    case CVChannel::B:
    {
      msg1 |= k_writeChannelB;
    }
    default:
    {
      return;
    }
  }

  digitalWriteFast(k_pinChipSelectDAC, LOW);
  SPI.transfer(msg1);
  SPI.transfer(msg2);
  digitalWriteFast(k_pinChipSelectDAC, HIGH);
}

//--------------------------------------------------------------------------------------------------

Synapse::Range Synapse::getCVRange(CVChannel channel_)
{
  switch (channel_)
  {
    case CVChannel::A:
    {
      return m_channelRange[0];
    }
    case CVChannel::B:
    {
      return m_channelRange[1];
    }
    default:
    {
      return Range::Unknown;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Synapse::setCVRange(CVChannel channel_, Range range_)
{
  switch (channel_)
  {
    case CVChannel::A:
    {
      m_channelRange[0] = range_;
    }
    case CVChannel::B:
    {
      m_channelRange[1] = range_;
    }
    default:
    {
      return;
    }
  }

  updateCVRanges();
}

//--------------------------------------------------------------------------------------------------

bool Synapse::readGate(GateChannel channel_)
{
  switch (channel_)
  {
    case GateChannel::A:
    {
      return digitalReadFast(k_pinGateInA);
    }
    case GateChannel::B:
    {
      return digitalReadFast(k_pinGateInB);
    }
    default:
    {
      return false;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Synapse::writeGate(GateChannel channel_, bool state_)
{
  switch (channel_)
  {
    case GateChannel::A:
    {
      digitalWriteFast(k_pinGateOutA, state_);
    }
    case GateChannel::B:
    {
      digitalWriteFast(k_pinGateOutB, state_);
    }
    default:
    {
      return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Synapse::gateInputInterrupt(GateChannel channel_, void (*callback_)(void), GateInterrupt mode_)
{
  switch (channel_)
  {
    case GateChannel::A:
    {
      attachInterrupt(digitalPinToInterrupt(k_pinGateInA), callback_, static_cast<uint32_t>(mode_));
    }
    case GateChannel::B:
    {
      attachInterrupt(digitalPinToInterrupt(k_pinGateInB), callback_, static_cast<uint32_t>(mode_));
    }
    default:
    {
      return;
    }
  }
}

//--------------------------------------------------------------------------------------------------

void Synapse::setSPIDivider(unsigned spiDivider_)
{
#ifdef __AVR__
  switch (spiDivider_)
  {
    case SPI_CLOCK_DIV2:
    case SPI_CLOCK_DIV4:
    case SPI_CLOCK_DIV8:
    case SPI_CLOCK_DIV16:
    case SPI_CLOCK_DIV32:
    case SPI_CLOCK_DIV64:
    case SPI_CLOCK_DIV128:
    {
      m_spiDivider = spiDivider_;
      break;
    }
    default:
    {
      m_spiDivider = SPI_CLOCK_DIV8;
    }
  }
#else
  m_spiDivider = SPI_CLOCK_DIV8;
#endif
  SPI.setClockDivider(m_spiDivider);
}

//--------------------------------------------------------------------------------------------------

void Synapse::updateCVRanges()
{
  for (unsigned channel = 0; channel < k_numCVOutputs; channel++)
  {
    switch (m_channelRange[channel])
    {
      case Range::ZeroToTenVolts:
      {
        digitalWriteFast( (channel > 0 ? k_pinCVOutConfB : k_pinCVOutConfA), LOW);
        break;
      }
      case Range::MinusFiveToFiveVolts:
      {
        digitalWriteFast( (channel > 0 ? k_pinCVOutConfB : k_pinCVOutConfA), HIGH);
        break;
      }
      case Range::Unknown:
      default:
        break;
    }
  }
}

//--------------------------------------------------------------------------------------------------

} // namespace sl