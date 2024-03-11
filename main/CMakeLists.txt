# See the build system documentation in IDF programming guide
# for more information about component CMakeLists.txt files.

idf_component_register(SRCS main.c  time_util.c aws_iot.c  
    				   INCLUDE_DIRS  "."   
    				   EMBED_FILES certs/aws_root_ca.pem certs/certificate_pem.crt certs/private_pem.key )    

