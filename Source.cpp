#include <iostream>
#include <fstream>
#include <cctype>
#include <string>
#include <vector>
#include <map>
#include <sstream>

using namespace std;


// ������������ ����� �������
enum TokenType {
    T_INT, T_FOR, T_RETURN, T_ID, T_NUM,
    T_EQ, T_PLUS, T_MINUS,
    T_LT, T_GT, T_LE, T_GE, T_EQEQ, T_NEQ,
    T_LPAREN, T_RPAREN, T_LBRACE, T_RBRACE,
    T_SEMI, T_COMMA, T_EOF, T_UNKNOWN
};

// �����: ��� ������, ��������� �������������, ����� ������ ������
struct Token {
    TokenType type;
    string text;
    int line;
};

// ������ �������� ��� �� ����� � ��������� ��� �� ������
class Lexer {
public:

    // �����������. ��������� ������� ���� � ��������������
    Lexer(const string& filename) {
        fin.open(filename);
        line = 1;
        nextChar();
        nextToken();
    }

    Token currentToken() { return curToken; }
    void nextToken() {
        skipSpaces();
        if (c == EOF) {
            curToken = { T_EOF,"",line };
            return;
        }

        if (isalpha(c)) {
            string id;
            while (isalnum(c) || c == '_') { id.push_back((char)c);nextChar(); }
            if (id == "int") curToken = { T_INT,id,line };
            else if (id == "for") curToken = { T_FOR,id,line };
            else if (id == "return") curToken = { T_RETURN,id,line };
            else curToken = { T_ID,id,line };
            return;
        }

        if (isdigit(c)) {
            string num;
            while (isdigit(c)) { num.push_back((char)c);nextChar(); }
            curToken = { T_NUM,num,line };
            return;
        }

        switch (c) {
        case '=': {
            nextChar();
            if (c == '=') { nextChar();curToken = { T_EQEQ,"==",line }; }
            else curToken = { T_EQ,"=",line };
        } break;
        case '+': nextChar();curToken = { T_PLUS,"+",line };break;
        case '-': nextChar();curToken = { T_MINUS,"-",line };break;
        case '<': {
            nextChar();
            if (c == '=') { nextChar();curToken = { T_LE,"<=",line }; }
            else curToken = { T_LT,"<",line };
        } break;
        case '>': {
            nextChar();
            if (c == '=') { nextChar();curToken = { T_GE,">=",line }; }
            else curToken = { T_GT,">",line };
        } break;
        case '!': {
            nextChar();
            if (c == '=') { nextChar();curToken = { T_NEQ,"!=",line }; }
            else { curToken = { T_UNKNOWN,"!",line }; }
        } break;
        case '(': nextChar();curToken = { T_LPAREN,"(",line };break;
        case ')': nextChar();curToken = { T_RPAREN,")",line };break;
        case '{': nextChar();curToken = { T_LBRACE,"{",line };break;
        case '}': nextChar();curToken = { T_RBRACE,"}",line };break;
        case ';': nextChar();curToken = { T_SEMI,";",line };break;
        case ',': nextChar();curToken = { T_COMMA,",",line };break;
        default: {
            char cc = (char)c;
            nextChar();
            curToken = { T_UNKNOWN,string(1,cc),line };
        } break;
        }
    }

    bool good() { return fin.good() || c != EOF; }

private:
    ifstream fin;
    int c;
    int line;
    Token curToken;

    void nextChar() {
        c = fin.get();
        if (c == '\n') line++;
    }

    void skipSpaces() {
        while (isspace(c) && c != EOF) {
            if (c == '\n') line++;
            c = fin.get();
        }
    }
};

enum VarType { VT_INT, VT_UNDEFINED };


// ������ ���������� � ����������, �� ����� � ������������ �������
class SymbolTable {
public:

    // ��������� ���������� � �������
    void declareVar(const string& name, VarType t) {
        if (vars.count(name)) {
            addError("Variable " + name + " redeclared");
        }
        else {
            vars[name] = t;
            declaredVars.push_back(name);
        }
    }

    // ���������� ��� ����������
    VarType getType(const string& name) {
        if (!vars.count(name)) {
            addError("Variable " + name + " not declared before use");
            return VT_UNDEFINED;
        }
        return vars[name];
    }

    // ��������� ��������� �� ������
    void addError(const string& err) {
        errors.push_back(err);
    }

    // ������ � ������ ������
    bool hasErrors() { return !errors.empty(); }
    const vector<string>& getErrors() { return errors; }

    // ������ ����������� ���������
    const vector<string>& getDeclaredVars() { return declaredVars; }

private:
    map<string, VarType> vars;
    vector<string> errors;
    vector<string> declaredVars;
};


// ���� � ������ ������
class Parser {
public:

    /*
    lex: ������ �� ������ Lexer (������������������ �������)
    sym: ������ �� ������ SymbolTable ��� ���������� ����������� � ������
    labelCount: ������� ��� ��������� ���������� �����
    currentFunctionType: ��� ������������� �������� ������� �������
    */
    Parser(Lexer& lex, SymbolTable& sym, ostream& out)
        :lex(lex), sym(sym), out(out) {
        next();
        labelCount = 1;
        currentFunctionType = VT_UNDEFINED;
    }

    void parse() {
        parseFunction();
        if (sym.hasErrors()) {
            out << "\nErrors:\n";
            for (auto& e : sym.getErrors()) out << e << "\n";
        }
    }

private:
    Lexer& lex;
    SymbolTable& sym;
    ostream& out;
    Token cur;
    int labelCount;
    VarType currentFunctionType;

    string newLabel() {
        ostringstream oss;oss << "m" << labelCount++;
        return oss.str();
    }

    void next() {
        lex.nextToken();
        cur = lex.currentToken();
    }

    void expect(TokenType t) {
        if (cur.type != t) {
            sym.addError("Expected token " + tokenName(t) + " got " + tokenName(cur.type));
        }
        else {
            next();
        }
    }

    string tokenName(TokenType t) {
        switch (t) {
        case T_INT:return "int";case T_FOR:return "for";case T_RETURN:return "return";
        case T_ID:return "identifier";case T_NUM:return "number";case T_EQ:return "=";
        case T_PLUS:return "+";case T_MINUS:return "-";
        case T_LT:return "<";case T_GT:return ">";case T_LE:return "<=";case T_GE:return ">=";
        case T_EQEQ:return "==";case T_NEQ:return "!=";
        case T_LPAREN:return "(";case T_RPAREN:return ")";case T_LBRACE:return "{";case T_RBRACE:return "}";
        case T_SEMI:return ";";case T_COMMA:return ",";case T_EOF:return "EOF";
        default:return "unknown";
        }
    }

    VarType parseType() {
        if (cur.type == T_INT) {
            next();
            return VT_INT;
        }
        else {
            // ���� ��������� ����������� ���, ������ ������� ��� int, �� ������ "unknown type"
            // �� ����� �������� � ���������� �� ������ ����, ��� ��� �������������� ������ int.
            // ��� ������������:
            return VT_INT;
        }
    }

    string parseId() {
        if (cur.type == T_ID) {
            string name = cur.text;
            next();
            return name;
        }
        else {
            sym.addError("Expected identifier");
            return "";
        }
    }

    void parseFunction() {
        VarType ftype = parseType(); // ������ ���� �������
        currentFunctionType = ftype; // ���������� ���� �������
        string fname = parseId(); // ��� �������
        expect(T_LPAREN); // �������� (
        expect(T_RPAREN); // )
        expect(T_LBRACE); // {

        // ���� �������������
        parseDescriptions();

        {
            const auto& vars = sym.getDeclaredVars();
            int count = (int)vars.size();
            out << "int ";
            for (auto& v : vars) out << v << " ";
            out << count + 1 << " DECL\n";
        }

        // ���� ��
        parseOperators();

        expect(T_RETURN); // �������� ������
        string retVar = parseId(); // ������ ������������ ����������
        expect(T_SEMI); // �������� ;
        expect(T_RBRACE); // }

        VarType rt = sym.getType(retVar); // �������� ���� ������������ ����������
        if (rt != currentFunctionType && rt != VT_UNDEFINED && currentFunctionType != VT_UNDEFINED) {
            sym.addError("Return type does not match function type");
        }

        // out << retVar << " return\n";        /////////////////////////////////////////////////////////////////////////////////////////////////
    }

    // ���� ������������
    void parseDescriptions() {
        while (cur.type == T_INT) { // �������� ���������� �� � ���
            parseDescr();
        }
    }

    // ���� �����
    void parseDescr() {
        VarType t = parseType(); // ��� ����������
        parseVarList(t); // ������ ������ ����������
        expect(T_SEMI); // �������� ������� ;
    }

    // ��������� ������ ����������
    void parseVarList(VarType t) {
        string name = parseId(); // ��� ����������
        if (!name.empty()) sym.declareVar(name, t); // ���������� ���������� � ������� ��������
        while (cur.type == T_COMMA) { // ���� ���� �������, �� ���������� ������
            next();
            string n = parseId();
            if (!n.empty()) sym.declareVar(n, t);
        }
    }

    // ���� ���������
    void parseOperators() {
        while (isStartOp()) { // ���� ����������� ���������� ���������
            parseOp();
        }
    }

    // �������� �� ���������� ��������� (id ��� ���������� ��� for ��� �����)
    bool isStartOp() {
        return cur.type == T_ID || cur.type == T_FOR;
    }

    // ������ ���������
    void parseOp() {
        // ����������
        if (cur.type == T_ID) {
            string var = parseId(); // ������ ����� ����������
            expect(T_EQ); // �������� =
            VarType et;
            string expr = parseExpr(et); // ������ ���������
            VarType vt = sym.getType(var); // �������� ���� ����������
            if (vt != et && vt != VT_UNDEFINED && et != VT_UNDEFINED) {
                sym.addError("Type mismatch in assignment to " + var);
            }
            expect(T_SEMI);
            out << expr << " " << var << " =\n"; // ��� ����������� ������
        }
        // ����
        else if (cur.type == T_FOR) { 
            parseFor();
        }
        else {
            sym.addError("Unexpected operator");
            // Skip
            while (cur.type != T_SEMI && cur.type != T_RBRACE && cur.type != T_EOF) next();
            if (cur.type == T_SEMI) next();
        }
    }

    // ������ �����
    void parseFor() {
        expect(T_FOR);
        expect(T_LPAREN);
        // 1. �������������
        string loopVar = parseId(); // ��� ����������-�������� // ��� ���������� �����
        expect(T_EQ);
        VarType initT;
        string initExpr = parseExpr(initT); // ��������� ��������
        expect(T_SEMI);
        out << initExpr << " " << loopVar << " =\n"; // ����������� ������ ��� �������������

        // 2. �������
        string startLabel = newLabel(); // ����� ������ �����
        string endLabel = newLabel(); // ����� ����� �����

        out << startLabel << " DEFL\n";

        VarType condT;
        string condExpr = parseCondition(condT); // ������� �����
        expect(T_SEMI);
        out << condExpr << " " << endLabel << " BRL\n";

        // 3. ���������
        VarType incT;
        string incExpr = parseExpr(incT); // ���������
        expect(T_RPAREN);
        expect(T_LBRACE);

        parseOperators(); // ���� �����

        expect(T_RBRACE);

        // 4. ���������� ��� ���������
        out << incExpr << "\n"; // ��������� 
        out << startLabel << " BRL\n"; // ������� � ������
        out << endLabel << " DEFL\n"; // ����� �����
    }

    // ������� �����
    string parseCondition(VarType& ct) {
        VarType lt, rt;
        // 1. ������ ������ ��������
        string left = parseExpr(lt); // ����� �����
        // 2. c��������� ��������� ���������
        TokenType op = cur.type;
        string opText = cur.text;
        if (op == T_EQEQ || op == T_NEQ || op == T_LT || op == T_GT || op == T_LE || op == T_GE) {
            next();
            // 4. ������ ������� ��������:
            string right = parseExpr(rt); // ������ �����
            if (lt != VT_INT || rt != VT_INT) {
                sym.addError("Condition operands must be int");
            }
            ct = VT_INT;
            return left + " " + right + " " + opText;
        }
        else {
            sym.addError("Expected relational operator in condition");
            ct = VT_UNDEFINED;
            return left;
        }
    }

    string parseExpr(VarType& et) {
        VarType st;
        string left = parseSimpleExpr(st); // ������� �������� ���������
        et = st;
        if (cur.type == T_PLUS || cur.type == T_MINUS) { // + -
            TokenType op = cur.type;
            string opText = cur.text;
            next();
            VarType rt;
            string right = parseExpr(rt); // ����������� �����
            if ((st != VT_INT) || (rt != VT_INT)) {
                sym.addError("Arithmetic operands must be int");
            }
            et = VT_INT;
            return left + " " + right + " " + opText;
        }
        return left;
    }

    string parseSimpleExpr(VarType& et) {
        if (cur.type == T_ID) {
            string id = cur.text;next();
            VarType vt = sym.getType(id); // ��������� ���� ����������
            et = vt;
            return id;
        }
        else if (cur.type == T_NUM) {
            string num = cur.text;next();
            et = VT_INT;
            return num;
        }
        else if (cur.type == T_LPAREN) {
            next();
            VarType innerT;
            string expr = parseExpr(innerT); // ��������� � �������
            expect(T_RPAREN);
            et = innerT;
            return expr;
        }
        else {
            sym.addError("Unexpected token in expression");
            next();
            et = VT_INT;
            return "";
        }
    }
};

int main() {
    Lexer lex("input.txt");
    SymbolTable sym;
    ofstream fout("output.txt");
    if (!fout) {
        cerr << "Cannot open output.txt\n";
        return 1;
    }
    Parser parser(lex, sym, fout);
    parser.parse();
    return 0;
}


/*
* ��� ������ �� 3 ����������:
*   1) ������ (���� � ������)
*   2) ����������� (�������� ���������� � ���������� � ������ + ������)
*   3) ������� (���� � ������.����������) (������ ��������� � ���������� ����������� ������)

*/