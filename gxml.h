#pragma once
#include <iostream>
#include <functional>
#include <cctype>
#include <cstdint>
#include <map>
#include <vector>
#include <sstream>


namespace gxml
{


    class CharGetter
    {
    public:

        inline CharGetter(std::function<bool()> hasNext, std::function<char()> next)
            :
            funcHasNext(hasNext),
            funcNext(next),
            anyCharLoaded(true)
        {
            c = 0;
            bHasNext = 0;


            if (!hasNext())
            {
                throw std::runtime_error("No Characters to parse");
            }

            this->loadNextChar();
        }

        inline void loadNextChar(void)
        {
            if (!funcHasNext())
            {
                throw std::runtime_error("No Characters to parse");
            }

            anyCharLoaded = true;
            c = funcNext.operator()();
            bHasNext = funcHasNext.operator()();
        }

        inline auto peek() -> char
        {
            if (!anyCharLoaded)
                loadNextChar();

            return this->c;
        }

        inline void unloadCurrentChar()
        {
            anyCharLoaded = false;
        }

        inline auto hasNext() -> bool
        {
            return this->bHasNext;
        }

    private:

        std::function<bool()> funcHasNext;
        std::function<char()> funcNext;
        char c;
        bool bHasNext, anyCharLoaded;
    };

    class Token
    {
    public:

        inline Token(std::string content) :
            content(content),
            bIsEmpty(false)
        {

        }

        inline Token() {}

        inline const std::string& getTokenContent()
        {
            return this->content;
        }

        inline bool isTokenEmpty()
        {
            return this->bIsEmpty;
        }

        inline bool isContentAlphanumeric()
        {
            if (this->content.size() == 0)
                return false;

            return isalnum((uint8_t)this->content.at(0));
        }

        inline friend std::ostream& operator<<(std::ostream& out_stream, Token tok)
        {
            return out_stream << tok.getTokenContent();
        }

        inline bool isWhitespaceToken() const
        {
            if (this->content.size() != 1)
                return false;

            return isspace((uint8_t)this->content.at(0));
        }

    private:
        std::string content;
        bool bIsEmpty = true;
    };

    class Tokenizer
    {
    public:

        inline Tokenizer(CharGetter c_getter) :
            c_getter(c_getter)
        {

        }

        inline Tokenizer(std::function<bool()> hasNext, std::function<char()> next) :
            c_getter(hasNext, next)
        {

        }

        inline void loadNextToken()
        {
            this->anyTokenLoaded = true;
            std::string result_content;

            bool isTokenContinuing = true;
            while (isTokenContinuing)
            {
                char c = c_getter.peek();


                if (isalnum((uint8_t)c))
                {
                    result_content += c;
                }
                else if (result_content.size() == 0)
                {
                    result_content += c;

                    isTokenContinuing = false;
                }
                else
                {
                    break;
                }

            loop_iteration_end:

                if (c_getter.hasNext())
                {
                    c_getter.unloadCurrentChar();
                }
                else
                {
                    this->alreadyGotLastChar = true;
                    break;
                }

            }

            this->currentToken = Token(result_content);
        }

        inline void unloadCurrentToken()
        {
            this->anyTokenLoaded = false;
        }

        inline bool hasNextToken()
        {
            return c_getter.hasNext() || !alreadyGotLastChar;
        }

        inline Token peek()
        {
            if (!anyTokenLoaded)
                loadNextToken();

            return this->currentToken;
        }

    private:

        bool alreadyGotLastChar = false, anyTokenLoaded = true;
        Token currentToken;
        CharGetter c_getter;

    };

    class Tag
    {
    public:

        inline Tag(std::string textContent) :
            bIsText(true),
            bIsStartingTag(false),
            bIsEndingTag(false),
            bIsEmpty(false),
            tagTypename("_text")
        {
            attributeList["_textcnt"].operator  =(textContent);
        }

        inline Tag() :
            bIsText(false),
            bIsStartingTag(false),
            bIsEndingTag(false),
            bIsEmpty(true)
        {}

        inline Tag(std::string tagTypename, std::map<std::string, std::string> attributeList, bool isStartingTag, bool isEndingTag) :
            tagTypename(tagTypename),
            attributeList(attributeList),
            bIsStartingTag(isStartingTag),
            bIsEndingTag(isEndingTag),
            bIsEmpty(false),
            bIsText(false)
        {

        }

        inline std::string toString() const
        {
            if (bIsEmpty)
                return "<empty>";

            if (bIsText)
                return attributeList.at("_textcnt");

            std::string result = "<";


            if (!bIsStartingTag && bIsEndingTag)
                result += "/";

            result += this->tagTypename;

            for (auto attr : this->attributeList)
            {
                result += " " + attr.first + "=\"" + attr.second + "\"";
            }

            if (bIsStartingTag && bIsEndingTag)
                result += "/";

            return result + ">";
        }

        inline const ::std::string& getTagTypename() const
        {
            return this->tagTypename;
        }

        inline const std::map<std::string, std::string>& getAttributes() const
        {
            return this->attributeList;
        }

        inline bool isText() const
        {
            return this->bIsText;
        }

        inline bool isBeginningTag() const
        {
            return this->bIsStartingTag;
        }

        inline bool isEmptyTag() const
        {
            return this->bIsEmpty;
        }

        inline bool isEndingTag() const
        {
            return this->bIsEndingTag;
        }

        inline friend std::ostream& operator<<(std::ostream& out, const Tag& t)
        {
            return out << t.toString();
        }

    private:

        bool bIsText, bIsStartingTag, bIsEndingTag, bIsEmpty;
        std::string tagTypename;
        std::map<std::string, std::string> attributeList;

    };


    class TagStream
    {
    public:

        inline TagStream(std::function<bool()> hasNext, std::function<char()> next) :
            tokenizer(hasNext, next)
        {

        }

        inline void loadNextTag()
        {

            skipWhitespaces();

            Token t = tokenizer.peek();



            if (t.getTokenContent() == "<")
            {
                parseNormalTag();
            }

            else
            {
                parseText();
            }

        }

        inline bool hasNextTag()
        {
            return this->tokenizer.hasNextToken() || !alreadyConsumedLastToken;
        }

        inline const Tag& peek()
        {
            return this->currentTag;
        }

        inline void skipEmptyTag()
        {
            if (this->peek().isEmptyTag())
                loadNextTag();

        }

        inline void unloadCurrentTag()
        {
            this->currentTag = Tag(); // Replacing current tag with empty tag
        }

    private:

        inline void parseText()
        {
            std::string content;


            Token t = tokenizer.peek();
            while (hasNextTag() && t.getTokenContent() != "<")
            {
                content += t.getTokenContent();

                loadNextToken();

                t = tokenizer.peek();
            }

            this->currentTag = Tag(content);
        }

        inline void parseNormalTag()
        {
            if (hasNextTag())
                loadNextToken();

            if (tokenizer.peek().getTokenContent() == "/")
            {


                if (hasNextTag())
                    loadNextToken();

                std::string tagTypename = tokenizer.peek().getTokenContent();

                while (hasNextTag())
                {
                    if (tokenizer.peek().getTokenContent() == ">")
                        break;

                    loadNextToken();
                }


                if (hasNextTag())
                    loadNextToken();

                this->currentTag = Tag(tagTypename, std::map<std::string, std::string>(), false, true);

                return;
            }

            std::string tagTypename = tokenizer.peek().getTokenContent();
            std::map<std::string, std::string> attributes;

            bool isEndingTag = false, isTagTypeToken = true;

            while (hasNextTag())
            {
                if (tokenizer.peek().getTokenContent() == ">")
                    break;

                if (tokenizer.peek().getTokenContent() == "/")
                {
                    isEndingTag = true;
                    loadNextToken();

                    if (tokenizer.peek().getTokenContent() != ">")
                    {
                        throw std::runtime_error("Unexpected Token, '>' expected");
                    }

                    break;
                }

                if (tokenizer.peek().isContentAlphanumeric() && !isTagTypeToken)
                    parseAttribute(attributes);

                loadNextToken();
                isTagTypeToken = false;
            }

            loadNextToken();

            this->currentTag = Tag(tagTypename, attributes, true, isEndingTag);

        }

        inline void skipWhitespaces()
        {
            Token t = tokenizer.peek();

            if (t.isTokenEmpty())
            {
                loadNextToken();
                t = tokenizer.peek();
            }

            while (t.isWhitespaceToken() && tokenizer.hasNextToken())
            {

                loadNextToken();
                t = tokenizer.peek();
            }
        }

        inline void parseAttribute(std::map<std::string, std::string>& attributeList)
        {
            std::string name, content;

            skipWhitespaces();

            name = tokenizer.peek().getTokenContent();

            loadNextToken();
            skipWhitespaces();

            if (tokenizer.peek().getTokenContent() != "=")
                throw std::runtime_error("Unexpected Token, '=' expected");

            loadNextToken();
            skipWhitespaces();

            if (tokenizer.peek().getTokenContent() != "\"")
                throw std::runtime_error("Unexpected Token, '\"' expected");

            loadNextToken();

            while (tokenizer.peek().getTokenContent() != "\"" && hasNextTag())
            {
                Token t = tokenizer.peek();

                if (t.getTokenContent() != "\"")
                    content += t.getTokenContent();

                loadNextToken();

            }

            attributeList[name] = content;

        }

        inline void loadNextToken()
        {
            if (!hasNextTag())
                throw std::runtime_error("Tried to load out of bounds token while creating the tags");

            tokenizer.unloadCurrentToken();
            if (!tokenizer.hasNextToken())
            {
                alreadyConsumedLastToken = true;
            }
        }


    private:

        gxml::Tokenizer tokenizer;
        Tag currentTag;
        bool alreadyConsumedLastToken = false;

    };


    class Element
    {
    public:

        inline Element(Tag starting_tag, std::vector<Element> subElems = std::vector<Element>()) :
            sub_elements(subElems),
            attributes(starting_tag.getAttributes()),
            elementTypename(starting_tag.getTagTypename()),
            bIsText(starting_tag.isText())
        {

        }

        inline std::string toString(int indentation = 0)
        {
            std::ostringstream result;

            std::string newline = "\n\r";

            for (int i = 0; i < indentation; ++i)
                newline += "\t";

            if (this->bIsText)
            {
                return newline + this->attributes.at("_textcnt");
            }


            result << newline << "<" << this->elementTypename;

            for (auto attr : this->attributes)
            {
                result << " " << attr.first << "=" << '"' << attr.second << '"';
            }

            result << ">";

            for (auto sub_elem : this->sub_elements)
            {
                result << sub_elem.toString(indentation + 1);
            }

            result << newline << "</" << this->elementTypename << ">";

            return result.str();
        }

        inline friend std::ostream& operator<< (std::ostream& out, Element e)
        {
            return out << e.toString();
        }

        const std::vector<Element>& getSubElements() const
        {
            if (bIsText)
                throw std::runtime_error("Tried to get SubElems of a text");
            return this->sub_elements;
        }

        bool isText() const { return bIsText; }

        const std::string& getTagname() const
        {
            if(bIsText)
                throw std::runtime_error("Tried to get Tagname of a text");
            return elementTypename;
        }

        const std::map<std::string, std::string>& getAttributes() const
        {
            if (bIsText)
                throw std::runtime_error("Tried to get Attributes of a text");
            return attributes;
        }

        const std::string& getTextContent() const
        {
            if (!bIsText)
                throw std::runtime_error("Tried to get text content of a non-text");
            return attributes.at("_textcnt");
        }

    private:

        std::vector<Element> sub_elements;
        bool bIsText;
        std::string elementTypename;
        std::map<std::string, std::string> attributes;

    };


    std::vector<Element> getSubElems(TagStream& tagStream, std::string super_tag_name);

    inline Element getElem(TagStream& tagStream)
    {
        tagStream.skipEmptyTag();


        auto firstTag = tagStream.peek();

        if (firstTag.isText() || (firstTag.isBeginningTag() && firstTag.isEndingTag())) // This is for special cases when the whole Element is made out of one single tag
        {
            return Element(firstTag);
        }

        tagStream.loadNextTag();

        auto sub_elems = getSubElems(tagStream, firstTag.getTagTypename());

        tagStream.unloadCurrentTag();

        return Element(firstTag, sub_elems);
    }


    inline std::vector<Element> getSubElems(TagStream& tagStream, std::string super_tag_name)
    {
        std::vector<Element> result;

        while (true)
        {

            if (tagStream.peek().isEndingTag() && !tagStream.peek().isBeginningTag() && !tagStream.peek().isText())
            {

                if (!(tagStream.peek().getTagTypename() == super_tag_name))
                    throw std::runtime_error("Unexpected ending Tag found, expected: " + super_tag_name + " found: " + tagStream.peek().getTagTypename());

                break;
            }

            result.push_back(getElem(tagStream));

            if (tagStream.hasNextTag())
                tagStream.loadNextTag();
            else
            {
                break;
            }

        }


        return result;
    }

}
