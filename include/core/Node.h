#ifndef NODE_H_
#define NODE_H_

#include <string>

using namespace std;

namespace dtn
{
namespace core
{
class ConvergenceLayer;

/**
 * Definiert den Knotentyp.
 * FLOATING Beschreibt einen Knoten welcher nur sporadisch verfügbar ist. Diese Art
 * von Knoten wird kontinuierlich auf Verfügbarkeit geprüft.
 * PERMANENT Beschreibt einen Knoten der dauerhaft verfügbar ist und daher keine
 * Kontrolle bedarf.
 */
enum NodeType
{
	FLOATING = 0,
	PERMANENT = 1
};

/**
 * Ein Node repräsentiert einen benachbarten Knoten welcher direkt erreicht werden kann.
 */
class Node
{
public:
	/**
	 * Konstruktor
	 * @param type Bestimmt den Knotentypen
	 * @sa NoteType
	 */
	Node(NodeType type = PERMANENT);

	/**
	 * Destruktor
	 */
	virtual ~Node();

	/**
	 * Liefert den Typ des Knoten zurück
	 * @return Den Typ des Knoten
	 * @sa NoteType
	 */
	NodeType getType() const;

	/**
	 * Setzt die für den ConvergenceLayer benötigte Adresse (z.B. für IP 10.0.0.1)
	 * @param address Die Adresse als String
	 */
	void setAddress(string address);

	/**
	 * Gibt die für den ConvergenceLayer benötigte Adresse des Knoten zurück
	 * @return Die Adresse des Knoten
	 */
	string getAddress() const;

	void setPort(unsigned int port);
	unsigned int getPort() const;

	/**
	 * Setzt eine Beschreibung für den Knoten
	 * @param description Die für den Knoten vorgesehene Beschreibung
	 */
	void setDescription(string description);

	/**
	 * Gibt eine Beschreibung für den Knoten zurück
	 * @return Eine Beschreibung für den Knoten als String
	 */
	string getDescription() const;

	/**
	 * Setz die URI des Knoten (In einem DTN Netzwerk sollte dies immer eine EID sein)
	 * @param uri Die URI des Knoten
	 */
	void setURI(string uri);

	/**
	 * Gibt die URI des Knoten zurück
	 * @return Die URI des Knoten
	 */
	string getURI() const;

	/**
	 * Setzt den Timeout des Knoten. Ein Timeout bestimmt das Intervall in dem ein
	 * Knoten antworten muss damit er als benachbarter Knoten in der Liste geführt wird.
	 * @param timeout Eine Anzahl von Millisekunden die einen Timeout definieren
	 */
	void setTimeout(int timeout);

	/**
	 * Gibt den Timeout in Millisekunden zurück
	 * @return Eine Anzahl von Millisekunden die einen Timeout definieren.
	 */
	int getTimeout() const;

	/**
	 * Dekrementiert den Timeout des Knoten
	 * @param step Den Wert um den der Timeout dekrementiert werden soll
	 * @return False, falls der Timeout abgelaufen ist, sonst True
	 */
	bool decrementTimeout(int step);

	/**
	 * Bestimmt ob ein Knoten zu einem anderen Knoten äquivalent ist
	 * @param node Ein Knoten der zu diesem Knoten äquivalent sein könnte
	 * @return True, wenn der übergebene Knoten mit diesem Knoten äquivalent ist
	 */
	 bool equals(Node &node);

	 ConvergenceLayer* getConvergenceLayer() const;
	 void setConvergenceLayer(ConvergenceLayer *cl);

	 void setPosition(pair<double,double> value);
	 pair<double,double> getPosition() const;

private:
	string m_address;
	string m_description;
	string m_uri;
	int m_timeout;
	NodeType m_type;
	unsigned int m_port;
	ConvergenceLayer *m_cl;

	pair<double,double> m_position;
};

}
}

#endif /*NODE_H_*/
