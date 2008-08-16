.PHONY: all
all: public private

#ifndef ND
ND=naturaldocs
#endif

# Note: In my version of naturaldocs, only one single -i option must be present,
# else it will wite absolute pathes everywhere.

.PHONY: public
public: public/html
	$(ND) --project public \
	    	-i ../../ \
		--output html public/html \
		--style Default CustomStyle

public/html:
	mkdir public/html

.PHONY: private
private: private/html
	$(ND) --project private \
	    	-i ../../ \
		--output html private/html \
		--style Default CustomStyle

private/html:
	mkdir private/html

.PHONY: clean
clean:
	rm -rf public/html public/Data
	rm -rf private/html private/Data
