.PHONY: all
all: public private

.PHONY: public
public: public/html
	NaturalDocs --project public \
	    	--input ../../src \
	    	--input ../../include \
		--output html public/html \
		--style Default CustomStyle

public/html:
	mkdir public/html

.PHONY: private
private: private/html
	NaturalDocs --project private \
	    	--input ../../src \
	    	--input ../../include \
		--output html private/html \
		--style Default CustomStyle

private/html:
	mkdir private/html

.PHONY: clean
clean:
	rm -rf public/html public/Data
	rm -rf private/html private/Data
