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

# Função para processar comandos e gerar respostas
def process_command(command):
    # Verifique se o comando possui o formato esperado
    if len(command) < 5 or command[0] != 0xAA:
        print("Formato de comando inválido")
        return None

    cmd_type = command[2]  # O tipo de comando é o terceiro byte
    print(f"Comando reconhecido: Tipo {cmd_type:02X}")

    # Gerar a resposta com base no tipo de comando
    if cmd_type == 0x01:  # Obter ESN
        response = bytearray([0xAA, 0x09, 0x01, 0x00, 0x23, 0x18, 0x60, 0x86, 0x7A])
    elif cmd_type == 0x03:  # Abortar transmissão
        response = bytearray([0xAA, 0x05, 0x03, 0x42, 0xF6])
    elif cmd_type == 0x06:  # Configuração
        response = bytearray([0xAA, 0x05, 0x06, 0xEF, 0xA1])
    elif cmd_type == 0x00:  # Enviar dados
        response = bytearray([0xAA, 0x05, 0x00, 0xD9, 0xC4])
    elif cmd_type == 0x04:  # Consultar estado do burst
        response = bytearray([0xAA, 0x06, 0x04, 0x00, 0xF4, 0x33])
    else:
        print(f"Comando desconhecido: Tipo {cmd_type:02X}")
        return bytearray([0xAA, 0xFF, 0xFF])  # Resposta genérica para comando não reconhecido
    
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
