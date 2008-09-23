.PHONY: all
all: public private

#ifndef ND
ND=NaturalDocs
#endif

EX=docs misc build scons_build

# Note: In my version of naturaldocs, only one single -i option must be present,
# else it will write absolute pathes everywhere.

XIOPTIONS=$(addprefix -xi ../../, $(EX))

.PHONY: public
public: public/html
	$(ND) --project public \
	    	-i ../../ \
	    	$(XIOPTIONS) \
		--output html public/html \
		--style Default CustomStyle

public/html:
	mkdir public/html

.PHONY: private
private: private/html
	$(ND) --project private \
	    	-i ../../ \
	    	$(XIOPTIONS) \
		--output html private/html \
		--style Default CustomStyle

private/html:
	mkdir private/html

.PHONY: clean
clean:
	rm -rf public/html public/Data
	rm -rf private/html private/Data
