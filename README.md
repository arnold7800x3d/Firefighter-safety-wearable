# Makerspace Firefigter Safety Wearable
## Project Summary
This repository contains the documentation for the implementation of the wearable intended to collect information pertinent to firefighters in the line of duty. This information includes:
1. Temperature
2. Humidity
3. Heartbeat
4. Location info (latitude and longitude)

## IoT Components
- Arduino MKR GSM 1400
- DHT22
- Neo GSM module
- MAX 30102 module

## MQTT Architecture and setup
The MQTT server is Mosquitto MQTT server. This is because it is open source and thus offers full control over the communication protocol of the system. The server is running on a Debian 13 virtual machine hosted on Oracle VirtualBox. The server setup is detailed as shown below:
1. Update the server and install Mosquitto
2. Create a root CA private key
```
openssl genrsa -out ca.key 2048
```
3. Generate self-signed CA certificate
```
openssl req -x509 -new -nodes -key ca.key -sha256 -days 365 -out ca.crt -subj "/C=KE/ST=Nairobi/L=Nairobi/O=Makserspace/OU=Research/CN=MakerspaceRootCA"
```
4. Create a server key
```
openssl genrsa -out server.key 2048
```
4. Generate a server certificate signing request
```
openssl req -new -key server.key -out server.csr -subj "/C=KE/ST=Nairobi/L=Nairobi/O=Makerspace/OU=Research/CN=192.168.100.24"
```
5. Create a config file for Subject Alternative Name (SAN)
server.ext
```
authorityKeyIdentifier=keyid,issuer
basicConstraints=CA:FALSE
keyUsage = digitalSignature, nonRepudiation, keyEncipherment, dataEncipherment
subjectAltName = IP:192.168.100.24

```
6. Sign the server CSR with the root CA cert adn create the server certificate
```
openssl x509 -req -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt -days 365 -sha256 -extfile server.ext
```

## 
mosquitto_sub -h localhost -t firefighterWearable/data -v --cafile /etc/mosquitto/certs/ca.crt -u Arnold -P 123Pass. -p 8883 --insecure