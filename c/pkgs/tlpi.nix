{
  lib,
  stdenv,
  fetchzip,
  libcap,
  acl,
  libselinux,
  libxcrypt,
  libseccomp,
}:
stdenv.mkDerivation rec {
  pname = "tlpi-dist";
  version = "240913";

  src = fetchzip {
    url = "https://man7.org/tlpi/code/download/tlpi-${version}-dist.tar.gz";
    hash = "sha256-z48klp7zOqtvOd1UJBi0CxHWp9DnAwa+fG0DHdC7iIg=";
  };

  # build dependencies
  nativeBuildInputs = [
    libcap
    acl
    libselinux

    # added in new version
    libxcrypt  # libcrypt-dev in debian
    libseccomp
  ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/{lib,include}

    cd lib/
    make

    install -Dm 755 ../libtlpi.a $out/lib/
    install -Dm 644 *.h $out/include/

    runHook postInstall
  '';

  meta = with lib; {
    description = "Source Code for The Linux Programming Interface - Distribution version";
    homepage = "https://man7.org/tlpi/code/index.html";
    platforms = platforms.linux;
  };
}
