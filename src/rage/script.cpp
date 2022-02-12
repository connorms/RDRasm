#include "script.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QSysInfo>

#include "../util/util.h"
#include "../../lib/Qt-AES/qaesencryption.h"

#define ReadVar(x) stream >> x;
#define ReadPointer(x) stream >> x; x = x & 0xffffff;

Script::Script(QString path)
    : m_script(path)
{
    if (!m_script.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "Error", "Error: Unable to open script.");
        return;
    }

    m_data = m_script.readAll();

    // Decompress and unencrypt script from inside resource file
    readRSCHeader();
    extractData();

    // Begin disassembling script once extracted from resource file
    int headerPos = findScriptHeader();

    if (headerPos == 0)
    {
        // TODO: message box with retry option, due to occasional decompression errors
        QMessageBox::critical(nullptr, "Error", "Error: Unable to find script header.");
        return;
    }

    readScriptHeader(headerPos);
    getOpcodes();
}

void Script::readRSCHeader()
{
    // Read data from header, which is uncompressed and unencrypted

    QDataStream stream(m_data);

    stream >> m_header.magic;

    stream >> m_header.version;
    stream >> m_header.flags1;
    stream >> m_header.flags2;

    m_header.vSize = (int)(m_header.flags2 & 0x7FFF);
    m_header.pSze = (int)((m_header.flags2 & 0xFFF7000) >> 14);
    m_header._f14_30 = (int)(m_header.flags2 & 0x70000000);
    m_header.extended = (m_header.flags2 & 0x80000000) == 0x80000000 ? true : false;
}

QByteArray Script::getData()
{
    return m_data;
}

QList<OpcodeBase> Script::getOpcodes()
{
    QFile file(":/opcodes/opcodes.json");

    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(nullptr, "Error", "Error: failed to open opcode template.");
        return QList<OpcodeBase>();
    }

    QJsonDocument document(QJsonDocument::fromJson(file.readAll()));

    QList<OpcodeBase> opcodes;

    foreach(const QJsonValue &v, document.object().value("opcodes").toArray())
    {
        QJsonObject obj = v.toObject();

        opcodes.push_back(OpcodeBase(obj.value("name").toString(), obj.value("size").toInt()));
    }

    return opcodes;
}

void Script::extractData()
{
    if (m_header.version == 2)
    {
        // remove header, since it's unneeded after being read and processed already
        m_data = m_data.remove(0, 16);

        // pad to nearest 16 bytes for AES to be able to decrypt
        m_data.resize(m_data.size() + (16 - (m_data.size() % 16)));

        for (int i = 0; i < 0x10; i++)
        {
            m_data = QAESEncryption::Decrypt(QAESEncryption::Aes::AES_256, QAESEncryption::Mode::ECB, m_data, Util::getAESKey());
        }

        m_data = m_data.remove(0, 8);

        int outsize = m_header.getSizeP() + m_header.getSizeV();

        unsigned char *in  = reinterpret_cast<unsigned char *>(m_data.data());
        unsigned char *out = new unsigned char[outsize];

        Util::decompressBuffer(in, out, outsize);

        m_data = QByteArray::fromRawData(reinterpret_cast<char *>(out), outsize);
    }
}

int Script::findScriptHeader()
{
    int headerOffset = 0;

    QDataStream stream(m_data);

    while (headerOffset < m_data.size())
    {
        int magic;
        stream >> magic;

        if (magic == -1462287616)
        {
            break;
        }

        headerOffset += 4096;
        stream.device()->seek(headerOffset);
    }

    return headerOffset;
}

void Script::readScriptHeader(int headerPos)
{
    QDataStream stream(m_data);

    stream.device()->seek(headerPos);

    ReadVar(m_scriptHeader.magic);
    ReadPointer(m_scriptHeader.pageMapPtr);
    ReadPointer(m_scriptHeader.codePagesPtr);
    ReadVar(m_scriptHeader.codeSize);
    ReadVar(m_scriptHeader.paramCount);
    ReadVar(m_scriptHeader.staticsSize);
    ReadPointer(m_scriptHeader.staticsPtr);
    ReadVar(m_scriptHeader.globalsVers);
    ReadVar(m_scriptHeader.nativesSize);
    ReadPointer(m_scriptHeader.nativesPtr);
}
