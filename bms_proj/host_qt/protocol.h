#pragma once
// BMS-5S 化成 CAN 报文解析 (与固件 BSP/can_report.c 一致，小端字节序)
#include <cstdint>
#include <QString>

namespace bms {

// fm_state_t (与固件 formation.h 顺序一致)
inline QString stateName(uint8_t s)
{
    static const char *n[] = {
        "IDLE", "CC_CHARGE", "CV_CHARGE", "REST_AFTER_CHG",
        "CC_DISCHARGE", "REST_AFTER_DSG", "COMPLETE", "ERROR"
    };
    return (s < 8) ? QString::fromLatin1(n[s]) : QString("?");
}

inline QString errorName(uint8_t e)
{
    static const char *n[] = { "NONE","CELL_OV","CELL_UV","OVER_TEMP","SENSOR","TIMEOUT" };
    return (e < 6) ? QString::fromLatin1(n[e]) : QString("?");
}

constexpr uint32_t ID_STATUS  = 0x100;  // pack/current/state/temp/charged
constexpr uint32_t ID_CELLS_A = 0x101;  // cell1..4
constexpr uint32_t ID_CELLS_B = 0x102;  // cell5/discharged/error

/* 上位机 → 板子 命令帧 (8 字节 LE, [0]=opcode, [1..]=参数) */
constexpr uint32_t ID_CMD     = 0x200;
constexpr uint8_t  CMD_START      = 0x01;  // 无参
constexpr uint8_t  CMD_STOP       = 0x02;  // 无参
constexpr uint8_t  CMD_SET_CHG_I  = 0x03;  // [1:2] u16 mA
constexpr uint8_t  CMD_SET_DSG_I  = 0x04;  // [1:2] u16 mA
constexpr uint8_t  CMD_CLEAR_ERR  = 0x05;  // 无参

struct BmsData {
    uint16_t pack_mV       = 0;
    int16_t  current_mA    = 0;
    uint8_t  state         = 0;
    int8_t   temp_C        = 0;
    uint16_t charged_mAh   = 0;
    uint16_t discharged_mAh= 0;
    uint16_t cell_mV[5]    = {0,0,0,0,0};
    uint8_t  error         = 0;
    bool     valid         = false;   // 收到过至少一帧 STATUS
};

inline uint16_t rdU16le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }

// 把一帧 (id, payload) 合并进 d。返回 true 表示是 STATUS 帧 (1Hz 锚点)。
inline bool applyFrame(BmsData &d, uint32_t id, const uint8_t *b, int len)
{
    if (len < 8) return false;
    switch (id) {
    case ID_STATUS:
        d.pack_mV     = rdU16le(b);
        d.current_mA  = (int16_t)rdU16le(b + 2);
        d.state       = b[4];
        d.temp_C      = (int8_t)b[5];
        d.charged_mAh = rdU16le(b + 6);
        d.valid       = true;
        return true;
    case ID_CELLS_A:
        d.cell_mV[0] = rdU16le(b);
        d.cell_mV[1] = rdU16le(b + 2);
        d.cell_mV[2] = rdU16le(b + 4);
        d.cell_mV[3] = rdU16le(b + 6);
        return false;
    case ID_CELLS_B:
        d.cell_mV[4]      = rdU16le(b);
        d.discharged_mAh  = rdU16le(b + 2);
        d.error           = b[4];
        return false;
    default:
        return false;
    }
}

} // namespace bms
