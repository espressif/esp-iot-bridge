

exclude_examples=["spi_and_sdio_host/","test"]


for d in */; do
  if [[ ! "$exclude_examples" =~ "$d" ]]; then
    echo $d
  fi
done
