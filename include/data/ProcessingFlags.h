#ifndef PROCESSINGFLAGS_H_
#define PROCESSINGFLAGS_H_

namespace dtn
{
namespace data
{
	class ProcessingFlags
	{
		public:
			/**
			 * Konstruktor
			 */
			ProcessingFlags();
		
			/**
			 * Konstruktor
			 * @param value Den Startwert der Flags
			 */
			ProcessingFlags(unsigned int value);
			
			/**
			 * Destruktor
			 */
			~ProcessingFlags();
			
			/**
			 * Setzt ein Flag
			 * @param flag Die Nummer des Flags welcher gesetzt werden soll
			 * @param value Der Wert der gesetzt werden soll
			 */
			void setFlag(unsigned int flag, bool value);
			
			/**
			 * Gibt den Wert eines Flags zurück
			 * @param flag Die Nummer des Flags welcher abgefragt werden soll
			 * @return Der Wert des Flags
			 */
			bool getFlag(unsigned int flag);
			
			/**
			 * Gibt die Repräsentation der setzbaren Parameter zurück
			 * @return Einen Integerwert der alle setzbaren Parameter enthält
			 */ 
			unsigned int getValue();			
		
		private:
			unsigned int m_value;
	};
}
}

#endif /*PROCESSINGFLAGS_H_*/
