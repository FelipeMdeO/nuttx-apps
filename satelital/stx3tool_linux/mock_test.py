import serial
import time
import sys

# Defina a porta serial para simular o mock (por exemplo, /dev/pts/14)
SERIAL_PORT = '/dev/pts/14'
BAUDRATE = 9600
TIMEOUT = 1  # segundos

# Função para calcular o CRC16
def calculate_crc(data):
    crc = 0xFFFF
    for b in data:
        crc ^= b
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0x8408
            else:
                crc >>= 1
    return ~crc & 0xFFFF

# Função para processar comandos e gerar respostas com verificação de CRC
def process_command(command):

    if command[0] != 0xAA:
        print("Formato de comando inválido ou tamanho insuficiente")
        return None

    # Separar os dados e o CRC fornecido
    data = command[:-2]       # Todos os bytes exceto os últimos dois
    received_crc = command[-2] | (command[-1] << 8)  # CRC está nos últimos dois bytes (little endian)

    # Calcular o CRC dos dados recebidos
    calculated_crc = calculate_crc(data)

    # Verificar se o CRC calculado corresponde ao CRC recebido
    if calculated_crc != received_crc:
        print(f"CRC inválido: Calculado {calculated_crc:04X}, Recebido {received_crc:04X}")
        return None

    # Extrair o tipo de comando (terceiro byte)
    cmd_type = command[2]
    print(f"Comando reconhecido: Tipo {cmd_type:02X}")

    # Gerar a resposta com base no tipo de comando
    if cmd_type == 0x01:  # Obter ESN
        response = bytearray([0xAA, 0x09, 0x01, 0x00, 0x23, 0x18, 0x60])
    elif cmd_type == 0x03:  # Abortar transmissão
        response = bytearray([0xAA, 0x05, 0x03])
    elif cmd_type == 0x06:  # Configuração
        response = bytearray([0xAA, 0x05, 0x06])
    elif cmd_type == 0x00:  # Enviar dados
        response = bytearray([0xAA, 0x05, 0x00])
    elif cmd_type == 0x04:  # Consultar estado do burst
        response = bytearray([0xAA, 0x06, 0x04, 0x00])
    else:
        print(f"Comando desconhecido: Tipo {cmd_type:02X}")
        return bytearray([0xAA, 0xFF, 0xFF])  # Resposta genérica para comando não reconhecido

    # Opcional: Adicionar CRC à resposta antes de retornar
    crc_response = calculate_crc(response)
    response += crc_response.to_bytes(2, byteorder='little')

    return response

# Função principal do mock
def main():
    try:
        ser = serial.Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT)
        print(f"Mock iniciado na porta {SERIAL_PORT}")
    except serial.SerialException as e:
        print(f"Erro ao abrir a porta serial: {e}")
        sys.exit(1)

    while True:
        if ser.in_waiting:
            # Ler o comando enviado pelo cliente
            command = ser.read(ser.in_waiting)
            print(f"Recebido comando: {command.hex()}")

            # Processar o comando e gerar a resposta
            response = process_command(command)
            if response:
                ser.write(response)
                print(f"Enviado resposta: {response.hex()}")
            else:
                print("Nenhuma resposta gerada para o comando recebido")
        time.sleep(0.1)

if __name__ == "__main__":
    main()
