.PHONY:clean FORCE

srctree   := $(shell pwd)
target_pack_name = dsp_pack
target_pack_src = $(wildcard $(srctree)/*.c)

# need to build a exc to pack dsp bin
$(target_pack_name): FORCE
	@gcc $(target_pack_src) -o $@
clean:
	@rm -rf $(target_pack_name)