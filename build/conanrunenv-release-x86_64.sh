script_folder="/home/jorik/cpp_proj/iot_mon_v2/build"
echo "echo Restoring environment" > "$script_folder/deactivate_conanrunenv-release-x86_64.sh"
for v in GRPC_DEFAULT_SSL_ROOTS_FILE_PATH OPENSSL_MODULES
do
    is_defined="true"
    value=$(printenv $v) || is_defined="" || true
    if [ -n "$value" ] || [ -n "$is_defined" ]
    then
        echo export "$v='$value'" >> "$script_folder/deactivate_conanrunenv-release-x86_64.sh"
    else
        echo unset $v >> "$script_folder/deactivate_conanrunenv-release-x86_64.sh"
    fi
done


export GRPC_DEFAULT_SSL_ROOTS_FILE_PATH="/home/jorik/.conan2/p/b/grpc818aebdb9f01a/p/res/grpc/roots.pem"
export OPENSSL_MODULES="/home/jorik/.conan2/p/b/opensd67cfcc050699/p/lib/ossl-modules"