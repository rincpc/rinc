# cmake -DOUT=build_id.c -P buildid.cmake: this build's id (when it was built), for rinc_buildId. Apps'
# cached images (docs/spec/apps.md#compiled-apps) from another build are compiled again
string(TIMESTAMP built "%Y-%m-%dT%H:%M:%SZ" UTC)
file(WRITE ${OUT} "const char rinc_buildId[] = \"${built}\";\n")
