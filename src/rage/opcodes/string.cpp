#include "string.h"

void Op_SPush::read(QDataStream *stream)
{
    m_delete   = false;
    m_location = stream->device()->pos() - 1;

    byte b;

    *stream >> b;

    m_data.push_back(b);

    for (int i = 0; i < b; i++)
    {
        byte stringByte;
        *stream >> stringByte;

        m_data.push_back(stringByte);
    }

    m_size = 2 + b;
}

QString Op_SPush::getFormattedBytes()
{
    // ignore string in data array
    return QString::number(getOp()) + QString::number(getData()[0], 16).toUpper();
}

QString Op_SPush::getFormattedData()
{
    // only return string
    QString result(getData());

    result.remove(0, 1);

    result.replace(0x0A, "\\n");

    return "\"" + result + "\"";
}

void Op_SPushL::read(QDataStream *stream)
{
    m_delete   = false;
    // todo
}

OP_REGISTER(Op_SPush);
OP_REGISTER(Op_SPushL);
