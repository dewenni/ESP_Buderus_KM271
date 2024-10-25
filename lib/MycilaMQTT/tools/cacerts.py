#!/usr/bin/env python
#
# modified ESP32 x509 certificate bundle generation utility to run with platformio
#
# Converts PEM and DER certificates to a custom bundle format which stores just the
# subject name and public key to reduce space
#
# The bundle will have the format: number of certificates; crt 1 subject name length; crt 1 public key length;
# crt 1 subject name; crt 1 public key; crt 2...
#
# SPDX-FileCopyrightText: 2018-2022 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: Apache-2.0

########################################################################################################
# Certificate Bundle Generation from PsychicMqttClient (https://github.com/theelims/PsychicMqttClient) #
########################################################################################################

from __future__ import with_statement

from pathlib import Path
import os
import struct
import sys
import requests
from io import open

Import("env")

try:
    from cryptography import x509
    from cryptography.hazmat.backends import default_backend
    from cryptography.hazmat.primitives import serialization
except ImportError:
    env.Execute("$PYTHONEXE -m pip install cryptography")

output_dir = ".pio/data"
quiet = False

def download_cacert_file():
    cacert_url = env.GetProjectOption("custom_cacert_url", "")
    if cacert_url == "":
        raise Exception("Please specify the URL of the CA certificate bundle with the 'custom_cacert_url' option in platformio.ini")

    response = requests.get(cacert_url)
    if response.status_code == 200:
        with open(os.path.join(output_dir, "cacerts.pem"), "w", encoding="utf-8") as f:
            f.write(response.text)
        status('Certificate bundle downloaded to: %s' % os.path.join(output_dir, "cacerts.pem"))
    else:
        status('Failed to fetch the certificate bundle.')

def status(msg):
    """ Print status message to stderr """
    if not quiet:
        critical(msg)

def critical(msg):
    """ Print critical message to stderr """
    sys.stderr.write('cacerts.py: ')
    sys.stderr.write(msg)
    sys.stderr.write('\n')

class CertificateBundle:
    def __init__(self):
        self.certificates = []
        self.compressed_crts = []

    def add_from_path(self, crts_path):

        found = False
        for file_path in os.listdir(crts_path):
            found |= self.add_from_file(os.path.join(crts_path, file_path))

        if found is False:
            raise InputError('No valid x509 certificates found in %s' % crts_path)

    def add_from_file(self, file_path):
        try:
            if file_path.endswith('.pem'):
                status('Parsing certificates from %s' % file_path)
                with open(file_path, 'r', encoding='utf-8') as f:
                    crt_str = f.read()
                    self.add_from_pem(crt_str)
                    return True

            elif file_path.endswith('.der'):
                status('Parsing certificates from %s' % file_path)
                with open(file_path, 'rb') as f:
                    crt_str = f.read()
                    self.add_from_der(crt_str)
                    return True

        except ValueError:
            critical('Invalid certificate in %s' % file_path)
            raise InputError('Invalid certificate')

        return False

    def add_from_pem(self, crt_str):
        """ A single PEM file may have multiple certificates """

        crt = ''
        count = 0
        start = False

        for strg in crt_str.splitlines(True):
            if strg == '-----BEGIN CERTIFICATE-----\n' and start is False:
                crt = ''
                start = True
            elif strg == '-----END CERTIFICATE-----\n' and start is True:
                crt += strg + '\n'
                start = False
                self.certificates.append(x509.load_pem_x509_certificate(crt.encode(), default_backend()))
                count += 1
            if start is True:
                crt += strg

        if count == 0:
            raise InputError('No certificate found')

        status('Successfully added %d certificates' % count)

    def add_from_der(self, crt_str):
        self.certificates.append(x509.load_der_x509_certificate(crt_str, default_backend()))
        status('Successfully added 1 certificate')

    def create_bundle(self):
        # Sort certificates in order to do binary search when looking up certificates
        self.certificates = sorted(self.certificates, key=lambda cert: cert.subject.public_bytes(default_backend()))

        bundle = struct.pack('>H', len(self.certificates))

        for crt in self.certificates:
            """ Read the public key as DER format """
            pub_key = crt.public_key()
            pub_key_der = pub_key.public_bytes(serialization.Encoding.DER, serialization.PublicFormat.SubjectPublicKeyInfo)

            """ Read the subject name as DER format """
            sub_name_der = crt.subject.public_bytes(default_backend())

            name_len = len(sub_name_der)
            key_len = len(pub_key_der)
            len_data = struct.pack('>HH', name_len, key_len)

            bundle += len_data
            bundle += sub_name_der
            bundle += pub_key_der

        return bundle

class InputError(RuntimeError):
    def __init__(self, e):
        super(InputError, self).__init__(e)


def main():
    os.makedirs(output_dir, exist_ok=True)

    output = os.path.join(output_dir, "cacerts.bin")
    if os.path.exists(output):
        status('Certificate bundle already exists')
        return

    source = os.path.join(output_dir, "cacerts.pem")
    if not os.path.exists(source):
        download_cacert_file()

    bundle = CertificateBundle()
    bundle.add_from_file(source)

    with open(output, 'wb') as f:
        f.write(bundle.create_bundle())

    status('cacert bundle created: %s' % output)
try:
    main()
except InputError as e:
    print(e)
    sys.exit(2)
