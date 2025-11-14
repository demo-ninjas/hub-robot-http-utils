
# CA Root Certificate Data

To update the CA Root Certificates, follow these steps: 

1. Run the `make-ca-bundle.pl` script to download the root certificates from Mozilla (this script comes from the cURL repo)
2. (if needed) Create a virtual python environment + add the dependency in requirements.txt
3. Source the virtual python environment
4. Run the `gen_crt_bundle.py` script

This will generate a new `x509_crt_bundle` file that will be embedded with the library.
